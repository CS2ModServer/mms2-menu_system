/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Menu System
 * Written by Wend4r & komashchenko (Vladimir Ezhikov & Borys Komashchenko).
 * ======================================================

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <menusystem_plugin.hpp>
#include <globals.hpp>
#include <math.hpp>

#include <stdint.h>

#include <exception>
#include <functional>
#include <string>

#include <any_config.hpp>

#include <sourcehook/sourcehook.h>

#include <filesystem.h>
#include <igameeventsystem.h>
#include <inetchannel.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworkserializer.h>
#include <recipientfilter.h>
#include <serversideclient.h>
#include <shareddefs.h>
#include <tier0/commonmacros.h>
#include <mathlib/mathlib.h>

#include <usermessages.pb.h>
// #include <cstrike15_usermessages.pb.h>

#define EF_MENU EF_BONEMERGE | EF_BRIGHTLIGHT | EF_DIMLIGHT | EF_NOINTERP | EF_NOSHADOW | EF_NODRAW | EF_NORECEIVESHADOW | EF_BONEMERGE_FASTCULL | EF_ITEM_BLINK | EF_PARENT_ANIMATES

SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandHandle, const CCommandContext &, const CCommand &);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t &, ISource2WorldSession *, const char *);
SH_DECL_HOOK8(CNetworkGameServerBase, ConnectClient, SH_NOATTRIB, 0, CServerSideClientBase *, const char *, ns_address *, void *, C2S_CONNECT_Message *, const char *, const byte *, int, bool);
SH_DECL_HOOK1(CServerSideClientBase, ProcessRespondCvarValue, SH_NOATTRIB, 0, bool, const CCLCMsg_RespondCvarValue_t &);
SH_DECL_HOOK1_void(CServerSideClientBase, PerformDisconnection, SH_NOATTRIB, 0, ENetworkDisconnectionReason);

static MenuSystem_Plugin s_aMenuPlugin;
MenuSystem_Plugin *g_pMenuPlugin = &s_aMenuPlugin;

PLUGIN_EXPOSE(MenuSystem_Plugin, s_aMenuPlugin);

MenuSystem_Plugin::MenuSystem_Plugin()
 :  Logger(GetName(), [](LoggingChannelID_t nTagChannelID)
    {
    	LoggingSystem_AddTagToChannel(nTagChannelID, s_aMenuPlugin.GetLogTag());
    }, 0, LV_DETAILED, MENUSYSTEM_LOGGINING_COLOR),

    CBaseEntity_Helper(this),
    CBaseModelEntity_Helper(this),
    CBasePlayerController_Helper(this),
    CBaseViewModel_Helper(this),
    CBodyComponent_Helper(this),
    CCSPlayer_ViewModelServices_Helper(this),
    CCSPlayerBase_CameraServices_Helper(this),
    CCSPlayerPawnBase_Helper(this),
    CGameSceneNode_Helper(this),

    PathResolver(this),

    m_mapConVarCookies(DefLessFunc(const CUtlSymbolLarge)),
    m_mapLanguages(DefLessFunc(const CUtlSymbolLarge))
{
	// Game events.
	{
		Menu::GameEventSystem::AddHandler("player_team", {[&](const CUtlSymbolLarge &sName, IGameEvent *pEvent) -> bool
		{
			auto aPlayerSlot = pEvent->GetPlayerSlot("userid");

			if(aPlayerSlot == CPlayerSlot::InvalidIndex())
			{
				return false;
			}

			auto &aPlayerData = GetPlayerData(aPlayerSlot);

			const auto &vecMenuEntities = aPlayerData.GetMenuEntities();

			if(!vecMenuEntities.Count())
			{
				return false;
			}

			auto *pPlayerPawn = entity_upper_cast<CBasePlayerPawn *>(pEvent->GetPlayerPawn("userid"));

			if(!pPlayerPawn)
			{
				return false;
			}

			if(pEvent->GetBool("disconnect") || pEvent->GetBool("isbot"))
			{
				return false;
			}

			int iNewTeam = pEvent->GetInt("team"), 
			    iOldTeam = pEvent->GetInt("oldteam");

			if(iNewTeam <= TEAM_SPECTATOR)
			{
				AttachMenuEntitiesToEntity(pPlayerPawn, vecMenuEntities);
			}
			else
			{
				auto *pCSPlayerPawn = entity_upper_cast<CCSPlayerPawnBase *>(pPlayerPawn);

				AttachMenuEntitiesToCSPlayer(pCSPlayerPawn, vecMenuEntities);
			}

			return true;
		}});
	}

	// Chat commands.
	{
		Menu::ChatCommandSystem::AddHandler("menu", {[&](const CUtlSymbolLarge &sName, CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArguments) -> bool
		{
			CSingleRecipientFilter aFilter(aSlot);

			int iClient = aSlot.Get();

			Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

			auto &aPlayer = m_aPlayers[iClient];

			if(!aPlayer.IsConnected())
			{
				return false;
			}

			const auto &aPhrase = aPlayer.GetYourArgumentPhrase();

			if(aPhrase.m_pFormat && aPhrase.m_pContent)
			{
				for(const auto &sArgument : vecArguments)
				{
					SendTextMessage(&aFilter, HUD_PRINTTALK, 1, aPhrase.m_pContent->Format(*aPhrase.m_pFormat, 1, sArgument.Get()).Get());
				}
			}
			else
			{
				Logger::Warning("Not found a your argument phrase\n");
			}

			// Spawn & attach menus.
			{
				auto *pPlayerController = entity_upper_cast<CBasePlayerController *>(g_pEntitySystem->GetEntityInstance(CEntityIndex(iClient + 1)));

				if(!pPlayerController)
				{
					Logger::WarningFormat("Failed to get player entity. Client index is %d\n", iClient);

					return false;
				}

				CBasePlayerPawn *pPlayerPawn = CBasePlayerController_Helper::GetPawnAccessor(pPlayerController)->Get();

				if(!pPlayerPawn)
				{
					Logger::WarningFormat("Failed to get player pawn. Client index is %d\n", iClient);

					return false;
				}

				CUtlVector<CEntityInstance *> vecEntitites;

				SpawnMenuEntitiesByEntity(pPlayerPawn, &vecEntitites);

				uint8 iTeam = CBaseEntity_Helper::GetTeamNumAccessor(pPlayerController);

				auto *pCSPlayerPawn = entity_upper_cast<CCSPlayerPawnBase *>(pPlayerPawn);

				if(iTeam <= TEAM_SPECTATOR)
				{
					AttachMenuEntitiesToEntity(pPlayerPawn, vecEntitites);
					TeleportMenuEntitiesToCSPlayer(pCSPlayerPawn, vecEntitites);
				}
				else
				{
					AttachMenuEntitiesToCSPlayer(pCSPlayerPawn, vecEntitites);
				}

				aPlayer.GetMenuEntities().AddVectorToTail(vecEntitites);

				return true;
			}
		}});

		Menu::ChatCommandSystem::AddHandler("menu_clear", {[&](const CUtlSymbolLarge &sName, CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArguments) -> bool
		{
			auto &aPlayer = GetPlayerData(aSlot);

			if(!aPlayer.IsConnected())
			{
				return false;
			}

			auto &vecMenuEntities = aPlayer.GetMenuEntities();

			for(auto *pMenuEntity : vecMenuEntities)
			{
				m_pEntityManagerProviderAgent->PushDestroyQueue(pMenuEntity);
			}

			m_pEntityManagerProviderAgent->ExecuteDestroyQueued();
			vecMenuEntities.Purge();

				return true;
		}});
	}
}

bool MenuSystem_Plugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	Logger::MessageFormat("Starting %s plugin...\n", GetName());

	if(!InitGlobals(ismm, error, maxlen))
	{
		return false;
	}

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		CBufferStringGrowable<1024> sMessage;

		DumpGlobals(g_aEmbedConcat, sMessage);
		Logger::Detailed(sMessage);
	}

	MathLib_Init();
	ConVar_Register(FCVAR_RELEASE | FCVAR_GAMEDLL);

	if(!InitProvider(error, maxlen))
	{
		return false;
	}

	if(!LoadProvider(error, maxlen))
	{
		return false;
	}

	if(!InitSchema(error, maxlen))
	{
		return false;
	}

	if(!LoadSchema(error, maxlen))
	{
		return false;
	}

	if(!InitEntityManager(error, maxlen))
	{
		return false;
	}

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		CBufferStringGrowable<1024> sMessage;

		DumpEntityManager(g_aEmbedConcat, sMessage);
		Logger::Detailed(sMessage);
	}

	if(!ParseLanguages(error, maxlen))
	{
		return false;
	}

	if(!ParseTranslations(error, maxlen))
	{
		return false;
	}

	if(!RegisterGameFactory(error, maxlen))
	{
		return false;
	}

	SH_ADD_HOOK(ICvar, DispatchConCommand, g_pCVar, SH_MEMBER(this, &MenuSystem_Plugin::OnDispatchConCommandHook), false);
	SH_ADD_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &MenuSystem_Plugin::OnStartupServerHook, true);

	if(late)
	{
		auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

		if(pNetServer)
		{
			OnStartupServer(pNetServer, pNetServer->m_GameConfig, NULL);

			for(const auto &pClient : pNetServer->m_Clients)
			{
				if(pClient->IsConnected() && !pClient->IsFakeClient())
				{
					OnConnectClient(pNetServer, pClient, pClient->GetClientName(), &pClient->m_nAddr, NULL, NULL, NULL, NULL, 0, pClient->m_bLowViolence);
				}
			}
		}

		// Initialize a game resource & load menu spawn groups.
		{
			char sMessage[256];

			if(!RegisterGameResource(sMessage, sizeof(sMessage)) || !LoadSpawnGroups(sMessage, sizeof(sMessage)))
			{
				Logger::WarningFormat("%s\n", sMessage);
			}
		}
	}

	ismm->AddListener(static_cast<ISmmPlugin *>(this), static_cast<IMetamodListener *>(this));

	Logger::MessageFormat("%s started!\n", GetName());

	return true;
}

bool MenuSystem_Plugin::Unload(char *error, size_t maxlen)
{
	{
		auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

		if(pNetServer)
		{
			SH_REMOVE_HOOK_MEMFUNC(CNetworkGameServerBase, ConnectClient, pNetServer, this, &MenuSystem_Plugin::OnConnectClientHook, true);
		}
	}

	SH_REMOVE_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &MenuSystem_Plugin::OnStartupServerHook, true);

	UnhookGameEvents();

	ClearLanguages();
	ClearTranslations();

	if(!UnloadProvider(error, maxlen))
	{
		return false;
	}

	if(!DestroySchema(error, maxlen))
	{
		return false;
	}

	if(!UnloadEntityManager(error, maxlen))
	{
		return false;
	}

	if(!UnregisterNetMessages(error, maxlen))
	{
		return false;
	}

	if(!UnregisterSource2Server(error, maxlen))
	{
		return false;
	}

	if(!UnregisterGameFactory(error, maxlen))
	{
		return false;
	}

	if(!UnloadSpawnGroups(error, maxlen))
	{
		return false;
	}

	if(!UnregisterGameResource(error, maxlen))
	{
		return false;
	}

	if(!DestoryGlobals(error, maxlen))
	{
		return false;
	}

	ConVar_Unregister();

	// ...

	return true;
}

bool MenuSystem_Plugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool MenuSystem_Plugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

void MenuSystem_Plugin::AllPluginsLoaded()
{
	/**
	 * AMNOTE: This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

const char *MenuSystem_Plugin::GetAuthor()        { return META_PLUGIN_AUTHOR; }
const char *MenuSystem_Plugin::GetName()          { return META_PLUGIN_NAME; }
const char *MenuSystem_Plugin::GetDescription()   { return META_PLUGIN_DESCRIPTION; }
const char *MenuSystem_Plugin::GetURL()           { return META_PLUGIN_URL; }
const char *MenuSystem_Plugin::GetLicense()       { return META_PLUGIN_LICENSE; }
const char *MenuSystem_Plugin::GetVersion()       { return META_PLUGIN_VERSION; }
const char *MenuSystem_Plugin::GetDate()          { return META_PLUGIN_DATE; }
const char *MenuSystem_Plugin::GetLogTag()        { return META_PLUGIN_LOG_TAG; }

void *MenuSystem_Plugin::OnMetamodQuery(const char *iface, int *ret)
{
	if(!strcmp(iface, MENUSYSTEM_INTERFACE_NAME))
	{
		if(ret)
		{
			*ret = META_IFACE_OK;
		}

		return static_cast<IMenuSystem *>(this);
	}

	if(ret)
	{
		*ret = META_IFACE_FAILED;
	}

	return nullptr;
}

CGameEntitySystem **MenuSystem_Plugin::GetGameEntitySystemPointer() const
{
	return &g_pGameEntitySystem;
}

CBaseGameSystemFactory **MenuSystem_Plugin::GetFirstGameSystemPointer() const
{
	return GetGameDataStorage().GetGameSystem().GetFirstPointer();
}

CGameSystemEventDispatcher **MenuSystem_Plugin::GetGameSystemEventDispatcherPointer() const
{
	return GetGameDataStorage().GetGameSystem().GetEventDispatcher();
}

IGameEventManager2 **MenuSystem_Plugin::GetGameEventManagerPointer() const
{
	return reinterpret_cast<IGameEventManager2 **>(GetGameDataStorage().GetSource2Server().GetGameEventManagerPointer());
}

const ISample::ILanguage *MenuSystem_Plugin::GetLanguageByName(const char *psz) const
{
	auto iFound = m_mapLanguages.Find(FindLanguageSymbol(psz));

	return m_mapLanguages.IsValidIndex(iFound) ? &m_mapLanguages.Element(iFound) : nullptr;
}

ISample::IPlayerBase *MenuSystem_Plugin::GetPlayerBase(const CPlayerSlot &aSlot)
{
	return GetPlayer(aSlot);
}

IMenuSystem::IPlayer *MenuSystem_Plugin::GetPlayer(const CPlayerSlot &aSlot)
{
	return &GetPlayerData(aSlot);
}

MenuSystem_Plugin::CPlayer &MenuSystem_Plugin::GetPlayerData(const CPlayerSlot &aSlot)
{
	int iClient = aSlot.Get();

	Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

	return m_aPlayers[iClient];
}

bool MenuSystem_Plugin::Init()
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}

	return true;
}

void MenuSystem_Plugin::PostInit()
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}
}

void MenuSystem_Plugin::Shutdown()
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}
}

GS_EVENT_MEMBER(MenuSystem_Plugin, GameActivate)
{
	// Initialize a game resource.
	{
		char sMessage[256];

		if(!RegisterGameResource(sMessage, sizeof(sMessage)))
		{
			Logger::WarningFormat("%s\n", sMessage);
		}
	}

	// Load menu spawn groups.
	{
		if(!LoadSpawnGroups())
		{
			Logger::Warning("Failed to load the menu spawn groups\n");
		}
	}
}

GS_EVENT_MEMBER(MenuSystem_Plugin, GameDeactivate)
{
	// ...
}

GS_EVENT_MEMBER(MenuSystem_Plugin, ServerPostEntityThink)
{
	// Teleport menu entities of active spectate players each think.
	for(auto &aPlayer : m_aPlayers)
	{
		if(!aPlayer.IsConnected())
		{
			continue;
		}

		const auto &vecMenuEntities = aPlayer.GetMenuEntities();

		if(!vecMenuEntities.Count())
		{
			continue;
		}

		auto *pServerSideClient = aPlayer.GetServerSideClient();

		int iClient = pServerSideClient->GetPlayerSlot().Get();

		auto *pPlayerController = entity_upper_cast<CBasePlayerController *>(g_pEntitySystem->GetEntityInstance(CEntityIndex(iClient + 1)));

		if(!pPlayerController)
		{
			continue;
		}

		uint8 iTeam = CBaseEntity_Helper::GetTeamNumAccessor(pPlayerController);

		if(iTeam != TEAM_SPECTATOR)
		{
			continue;
		}

		CBasePlayerPawn *pPlayerPawn = CBasePlayerController_Helper::GetPawnAccessor(pPlayerController)->Get();

		if(!pPlayerPawn)
		{
			continue;
		}

		auto *pCSPlayerPawn = entity_upper_cast<CCSPlayerPawnBase *>(pPlayerPawn);

		TeleportMenuEntitiesToCSPlayer(pCSPlayerPawn, vecMenuEntities);
	}
}

void MenuSystem_Plugin::OnSpawnGroupAllocated(SpawnGroupHandle_t hSpawnGroup, ISpawnGroup *pSpawnGroup)
{
	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(hSpawnGroup = %d, pSpawnGroup = %p)\n", __FUNCTION__, hSpawnGroup, pSpawnGroup);
	}

	// AsyncSpawnMenuEntities();
}

void MenuSystem_Plugin::OnSpawnGroupInit(SpawnGroupHandle_t hSpawnGroup, IEntityResourceManifest *pManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry)
{
	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(hSpawnGroup = %d, pManifest = %p, pConfig = %p, pRegistry = %p)\n", __FUNCTION__, hSpawnGroup, pManifest, pConfig, pRegistry);
	}

	Assert(pManifest);

	m_pEntityManagerProviderAgent->AddResourceToEntityManifest(pManifest, "materials/dev/annotation_worldtext_background.vmat");
	m_pEntityManagerProviderAgent->AddResourceToEntityManifest(pManifest, "materials/editor/icon_empty.vmat");
}

void MenuSystem_Plugin::OnSpawnGroupCreateLoading(SpawnGroupHandle_t hSpawnGroup, CMapSpawnGroup *pMapSpawnGroup, bool bSynchronouslySpawnEntities, bool bConfirmResourcesLoaded, CUtlVector<const CEntityKeyValues *> &vecKeyValues)
{
	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(hSpawnGroup = %d, pMapSpawnGroup = %p, bSynchronouslySpawnEntities = %s, bConfirmResourcesLoaded = %s, &vecKeyValues = %p)\n", __FUNCTION__, hSpawnGroup, pMapSpawnGroup, bSynchronouslySpawnEntities ? "true" : "false", bConfirmResourcesLoaded ? "true" : "false", &vecKeyValues);
	}

	const Vector vecBackgroundOrigin {-42.0f, 30.0f, -159.875f}, 
	             vecOrigin {-42.0f, 30.0f, -160.0f};

	const QAngle angRotation {180.0f, 0.0f, 0.0f};

	const Vector vecScales {1.f, 1.f, 1.f};

	CEntityKeyValues *pMenuKV = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL),
	                 *pMenuKV2 = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL), 
	                 *pMenuKV3 = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL);

	FillMenuEntityKeyValues(pMenuKV, vecBackgroundOrigin, angRotation, vecScales, MENUSYSTEM_BACKGROUND_COLOR, MENUSYSTEM_DEFAULT_FONT_FAMILY, MENUSYSTEM_BACKGROUND_MATERIAL_NAME, "Title\n\n1. Active");
	FillMenuEntityKeyValues(pMenuKV2, vecOrigin, angRotation, vecScales, MENUSYSTEM_ACTIVE_COLOR, MENUSYSTEM_DEFAULT_FONT_FAMILY, MENUSYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, "\n\n1. Active");
	FillMenuEntityKeyValues(pMenuKV3, vecOrigin, angRotation, vecScales, MENUSYSTEM_INACTIVE_COLOR, MENUSYSTEM_DEFAULT_FONT_FAMILY, MENUSYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, "Title");

	g_pEntitySystem->AddRefKeyValues(pMenuKV);
	g_pEntitySystem->AddRefKeyValues(pMenuKV2);
	g_pEntitySystem->AddRefKeyValues(pMenuKV3);

	vecKeyValues.AddToTail(pMenuKV);
	vecKeyValues.AddToTail(pMenuKV2);
	vecKeyValues.AddToTail(pMenuKV3);
}

void MenuSystem_Plugin::OnSpawnGroupDestroyed(SpawnGroupHandle_t hSpawnGroup)
{
	m_pMySpawnGroupInstance->RemoveNotificationsListener(static_cast<IEntityManager::IProviderAgent::ISpawnGroupNotifications *>(this));
}

bool MenuSystem_Plugin::InitPathResolver(char *error, size_t maxlen)
{
	if(!PathResolver::Init())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to initialize a path resolver", maxlen);
		}
		return false;
	}

	m_sBaseGameDirectory = PathResolver::Extract();

	return true;
}

bool MenuSystem_Plugin::ClearPathResolver(char *error, size_t maxlen)
{
	PathResolver::Clear();

	return true;
}

bool MenuSystem_Plugin::InitProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Init(vecMessages);

	if(vecMessages.Count())
	{
		if(Logger::IsChannelEnabled(LS_WARNING))
		{
			auto aWarnings = Logger::CreateWarningsScope();

			FOR_EACH_VEC(vecMessages, i)
			{
				auto &aMessage = vecMessages[i];

				aWarnings.Push(aMessage.Get());
			}

			aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
			{
				Logger::Warning(rgba, sContext);
			});
		}
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to initialize provider. See warnings", maxlen);
		}
	}

	return bResult;
}

bool MenuSystem_Plugin::LoadProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Load(MENUSYSTEM_GAME_BASE_DIR, MENUSYSTEM_BASE_PATHID, vecMessages);

	if(vecMessages.Count())
	{
		if(Logger::IsChannelEnabled(LS_WARNING))
		{
			auto aWarnings = Logger::CreateWarningsScope();

			FOR_EACH_VEC(vecMessages, i)
			{
				auto &aMessage = vecMessages[i];

				aWarnings.Push(aMessage.Get());
			}

			aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
			{
				Logger::Warning(rgba, sContext);
			});
		}
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to load provider. See warnings", maxlen);
		}
	}

	return bResult;
}

bool MenuSystem_Plugin::UnloadProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Destroy(vecMessages);

	if(vecMessages.Count())
	{
		if(Logger::IsChannelEnabled(LS_WARNING))
		{
			auto aWarnings = Logger::CreateWarningsScope();

			FOR_EACH_VEC(vecMessages, i)
			{
				auto &aMessage = vecMessages[i];

				aWarnings.Push(aMessage.Get());
			}

			aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
			{
				Logger::Warning(rgba, sContext);
			});
		}
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unload provider. See warnings", maxlen);
		}
	}

	return bResult;
}

bool MenuSystem_Plugin::InitSchema(char *error, size_t maxlen)
{
	CUtlVector<const char *> vecLoadLibraries;

	vecLoadLibraries.AddToTail(
#if defined(_WINDOWS)
		"server.dll"
#elif defined(_LINUX)
		"libserver.so"
#elif defined(_OSX)
		"libserver.dylib"
#endif
	);

	Menu::Schema::CSystem::CBufferStringVector vecMessages;

	bool bResult = Menu::Schema::CSystem::Init(g_pSchemaSystem, vecLoadLibraries, &vecMessages);

	if(Logger::IsChannelEnabled(LS_WARNING))
	{
		auto aWarnings = Logger::CreateWarningsScope();

		FOR_EACH_VEC(vecMessages, i)
		{
			auto &aMessage = vecMessages[i];

			aWarnings.Push(aMessage.Get());
		}

		aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
		{
			Logger::Warning(rgba, sContext);
		});
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to initialize schema helper. See warnings", maxlen);
		}
	}

	return bResult;
}

bool MenuSystem_Plugin::LoadSchema(char *error, size_t maxlen)
{
	Menu::Schema::CSystem::CBufferStringVector vecMessages;

	Menu::Schema::CSystem::FullDetails_t aFullDetails 
	{
		{
			&vecMessages,
		},

		{
			&g_aEmbed5Concat,
			&g_aEmbed4Concat,
			&g_aEmbed3Concat,
			&g_aEmbed2Concat,
			&g_aEmbedConcat,
		}
	};

	bool bResult = Menu::Schema::CSystem::Load(&aFullDetails);

	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		auto aDetails = Logger::CreateDetailsScope("", "");

		for(const auto &sMessage : vecMessages)
		{
			aDetails.Push(sMessage.Get());
		}

		aDetails.SendColor([&](Color rgba, const CUtlString &sContext)
		{
			Logger::Detailed(rgba, sContext);
		});
	}

	return bResult;
}

bool MenuSystem_Plugin::DestroySchema(char *error, size_t maxlen)
{
	Menu::Schema::CSystem::Destroy();

	return true;
}

bool MenuSystem_Plugin::InitEntityManager(char *error, size_t maxlen)
{
	// Gets a main entity manager interface.
	{
		m_pEntityManager = reinterpret_cast<IEntityManager *>(g_SMAPI->MetaFactory(ENTITY_MANAGER_INTERFACE_NAME, nullptr, nullptr));

		if(!m_pEntityManager)
		{
			strncpy(error, "Failed to get a entity manager interface", maxlen);

			return false;
		}
	}

	// Gets an entity manager provider agent interface.
	{
		m_pEntityManagerProviderAgent = m_pEntityManager->GetProviderAgent();

		if(!m_pEntityManagerProviderAgent)
		{
			strncpy(error, "Failed to get a entity manager provider agent interface", maxlen);

			return false;
		}
	}

	// Gets an entity manager spawn group mgr interface.
	{
		m_pEntityManagerSpawnGroupProvider = m_pEntityManager->GetSpawnGroupProvider();

		if(!m_pEntityManagerSpawnGroupProvider)
		{
			strncpy(error, "Failed to get a entity manager spawn group mgr interface", maxlen);

			return false;
		}
	}

	return true;
}

void MenuSystem_Plugin::DumpEntityManager(const CConcatLineString &aConcat, CBufferString &sOutput)
{
	GLOBALS_APPEND_VARIABLE(aConcat, m_pEntityManager);
	GLOBALS_APPEND_VARIABLE(aConcat, m_pEntityManagerProviderAgent);
	GLOBALS_APPEND_VARIABLE(aConcat, m_pEntityManagerSpawnGroupProvider);
}

bool MenuSystem_Plugin::UnloadEntityManager(char *error, size_t maxlen)
{
	return true;
}

bool MenuSystem_Plugin::LoadSpawnGroups(char *error, size_t maxlen)
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}

	{
		CUtlString sMenu = GetName();

		SpawnGroupDesc_t aDesc;

		// aDesc.m_hOwner = 1; // Merge the spawn group into active one.
		// aDesc.m_sWorldName = sMenu;
		aDesc.m_sDescriptiveName = sMenu;
		aDesc.m_sEntityLumpName = "main lump";
		aDesc.m_sEntityFilterName = "menu_loader";
		aDesc.m_sLocalNameFixup = "menu_system";
		aDesc.m_sWorldGroupname = "menu";
		aDesc.m_manifestLoadPriority = RESOURCE_MANIFEST_LOAD_PRIORITY_HIGH;
		aDesc.m_bCreateClientEntitiesOnLaterConnectingClients = true;
		aDesc.m_bBlockUntilLoaded = true;

		auto *pSpawnGroupInstance = m_pEntityManagerProviderAgent->CreateSpawnGroup();

		pSpawnGroupInstance->AddNotificationsListener(static_cast<IEntityManager::IProviderAgent::ISpawnGroupNotifications *>(this));

		if(pSpawnGroupInstance->Load(aDesc, {0.f, 0.f, 0.f}))
		{
			m_pMySpawnGroupInstance = pSpawnGroupInstance;
		}
		else
		{
			snprintf(error, maxlen, "Failed to load \"%s\" spawn group", sMenu.Get());
		}
	}

	return true;
}

bool MenuSystem_Plugin::UnloadSpawnGroups(char *error, size_t maxlen)
{
	if(m_pEntityManagerProviderAgent && m_pMySpawnGroupInstance)
	{
		m_pEntityManagerProviderAgent->ReleaseSpawnGroup(m_pMySpawnGroupInstance);
	}

	return true;
}

void MenuSystem_Plugin::FillMenuEntityKeyValues(CEntityKeyValues *pMenuKV, const Vector &vecOrigin, const QAngle &angRotation, const Vector &vecScales, const Color rgbaColor,  const char *pszFontName, const char *pszBackgroundMaterialName, const char *pszMessageText)
{
	pMenuKV->SetString("classname", "point_worldtext");
	pMenuKV->SetVector("origin", vecOrigin);
	pMenuKV->SetQAngle("angles", angRotation);
	pMenuKV->SetVector("scales", vecScales);

	// Text settings.
	pMenuKV->SetBool("enabled", true);
	pMenuKV->SetBool("fullbright", true);
	pMenuKV->SetColor("color", rgbaColor);
	pMenuKV->SetFloat("world_units_per_pixel", 0.025f);
	pMenuKV->SetInt("font_size", 80);
	pMenuKV->SetString("font_name", pszFontName);
	pMenuKV->SetInt("justify_horizontal", 0);
	pMenuKV->SetInt("justify_vertical", 0);
	pMenuKV->SetFloat("depth_render_offset", 0.125f);
	pMenuKV->SetInt("reorient_mode", 0);

	// Background.
	pMenuKV->SetBool("draw_background", true);
	pMenuKV->SetString("background_material_name", pszBackgroundMaterialName);
	// pMenuKV->SetString("background_material_name", "materials/dev/point_worldtext_default_background.vmat");
	// pMenuKV->SetString("background_material_name", "materials/editor/icon_empty.vmat");
	pMenuKV->SetFloat("background_border_width", 2.0f);
	pMenuKV->SetFloat("background_border_height", 1.0f);
	pMenuKV->SetFloat("background_world_to_uv", 0.1f);

	pMenuKV->SetString("message", pszMessageText);
}

void MenuSystem_Plugin::FillViewModelEntityKeyValues(CEntityKeyValues *pEntityKV, const Vector &vecOrigin, const QAngle &angRotation)
{
	pEntityKV->SetString("classname", "viewmodel");
	pEntityKV->SetVector("origin", vecOrigin);
	pEntityKV->SetQAngle("angles", angRotation);
}

Vector MenuSystem_Plugin::GetEntityPosition(CBaseEntity *pEntity, QAngle *pRotation)
{
	CBodyComponent *pEntityBodyComponent = CBaseEntity_Helper::GetBodyComponentAccessor(pEntity);

	CGameSceneNode *pEntitySceneNode = CBodyComponent_Helper::GetSceneNodeAccessor(pEntityBodyComponent);

	if(pRotation)
	{
		*pRotation = CGameSceneNode_Helper::GetAbsRotationAccessor(pEntitySceneNode);
	}

	return CGameSceneNode_Helper::GetAbsOriginAccessor(pEntitySceneNode);
}

void MenuSystem_Plugin::CalculateMenuEntitiesPosition(const Vector &vecOrigin, const QAngle &angRotation, const float flPitchOffset, const float flYawOffset, const float flAddDistance, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	const QAngle angOffset {AngleNormalize(angRotation.x + flPitchOffset), AngleNormalize(angRotation.y + flYawOffset), 0.f};

	vecResult = AddToFrontByRotation2(vecOrigin, angOffset, flAddDistance);
	vecBackgroundResult = AddToFrontByRotation2(vecResult, angOffset, 0.04f);

	angResult = {0.f, AngleNormalize(angRotation.y - 90.f), AngleNormalize(-angRotation.x + 90.f)};
}

void MenuSystem_Plugin::CalculateMenuEntitiesPositionByEntity(CBaseEntity *pTarget, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	vecResult = GetEntityPosition(pTarget, &angResult);
	CalculateMenuEntitiesPosition(vecResult, angResult, sm_flPitchOffset, sm_flYawOffset, sm_flAddDistance, vecBackgroundResult, vecResult, angResult);
}

void MenuSystem_Plugin::CalculateMenuEntitiesPositionByViewModel(CBaseViewModel *pTarget, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	vecResult = GetEntityPosition(pTarget, &angResult);
	CalculateMenuEntitiesPosition(vecResult, angResult, 0.f, 0.f, sm_flAddDistance / 1.5f, vecBackgroundResult, vecResult, angResult);
}

void MenuSystem_Plugin::CalculateMenuEntitiesPositionByCSPlayer(CCSPlayerPawnBase *pTarget, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	vecResult = GetEntityPosition(pTarget) + CBaseModelEntity_Helper::GetViewOffsetAccessor(pTarget);
	angResult = CCSPlayerPawnBase_Helper::GetEyeAnglesAccessor(pTarget);
	CalculateMenuEntitiesPosition(vecResult, angResult, 0.f, 0.f, sm_flAddDistance / 1.5f, vecBackgroundResult, vecResult, angResult);
}

void MenuSystem_Plugin::SpawnEntities(const CUtlVector<CEntityKeyValues *> &vecKeyValues, CUtlVector<CEntityInstance *> *pEntities, IEntityManager::IProviderAgent::IEntityListener *pListener)
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}

	auto *pEntitySystemAllocator = g_pEntitySystem->GetEntityKeyValuesAllocator();

	const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

	COMPILE_TIME_ASSERT(INVALID_SPAWN_GROUP == ANY_SPAWN_GROUP);

	for(auto *pKeyValues : vecKeyValues)
	{
		m_pEntityManagerProviderAgent->PushSpawnQueue(pKeyValues, hSpawnGroup);
	}

	{
		CUtlVector<CUtlString> vecDetails, 
		                       vecWarnings;

		m_pEntityManagerProviderAgent->ExecuteSpawnQueued(hSpawnGroup, pEntities, pListener, &vecDetails, &vecWarnings);

		if(vecDetails.Count())
		{
			if(Logger::IsChannelEnabled(LS_DETAILED))
			{
				auto aDetails = Logger::CreateDetailsScope();

				for(const auto &it : vecDetails)
				{
					aDetails.Push(it);
				}

				aDetails.SendColor([&](Color rgba, const CUtlString &sContext)
				{
					Logger::Detailed(rgba, sContext);
				});
			}
		}

		if(vecWarnings.Count())
		{
			if(Logger::IsChannelEnabled(LS_WARNING))
			{
				auto aWarnings = Logger::CreateWarningsScope();

				for(const auto &it : vecWarnings)
				{
					aWarnings.Push(it);
				}

				aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
				{
					Logger::Warning(rgba, sContext);
				});
			}
		}
	}
}

void MenuSystem_Plugin::SpawnMenuEntities(const Vector &vecBackgroundOrigin, const Vector &vecOrigin, const QAngle &angRotation, CUtlVector<CEntityInstance *> *pEntities)
{
	auto *pEntitySystemAllocator = g_pEntitySystem->GetEntityKeyValuesAllocator();

	const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

	static_assert(INVALID_SPAWN_GROUP == ANY_SPAWN_GROUP);

	CEntityKeyValues *pMenuKV = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL),
	                 *pMenuKV2 = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL), 
	                 *pMenuKV3 = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL);

	CUtlVector<CEntityKeyValues *> vecKeyValues;

	const Vector vecScales {0.25f, 0.25f, 0.25f};

	static const char szMessageTextFull[] = "Заголовок\n"
	                                        "\n"
	                                        "1. bratbufi\n"
	                                        "2. mamabufi\n"
	                                        "3. doublebufi\n"
	                                        "4. bufi\n"
	                                        "5. megabufi\n"
	                                        "6. superbufi\n"
	                                        "\n"
	                                        "7. Назад\n"
	                                        "8. Вперёд\n"
	                                        "9. Выход\n",

	                  szMessageTextActive[] = "\n"
	                                          "\n"
	                                          "1. bratbufi\n"
	                                          "\n"
	                                          "3. doublebufi\n"
	                                          "4. bufi\n"
	                                          "\n"
	                                          "6. superbufi\n"
	                                          "\n"
	                                          "7. Назад\n"
	                                          "8. Вперёд\n"
	                                          "9. Выход\n",

	                  szMessageTextInactive[] = "Заголовок\n"
	                                            "\n"
	                                            "\n"
	                                            "2. mamabufi\n"
	                                            "\n"
	                                            "\n"
	                                            "5. megabufi\n"
	                                            "\n"
	                                            "\n"
	                                            "\n"
	                                            "\n"
	                                            "\n";

	FillMenuEntityKeyValues(pMenuKV, vecBackgroundOrigin, angRotation, vecScales, MENUSYSTEM_BACKGROUND_COLOR, MENUSYSTEM_DEFAULT_FONT_FAMILY, MENUSYSTEM_BACKGROUND_MATERIAL_NAME, szMessageTextFull);
	FillMenuEntityKeyValues(pMenuKV2, vecOrigin, angRotation, vecScales, MENUSYSTEM_ACTIVE_COLOR, MENUSYSTEM_DEFAULT_FONT_FAMILY, MENUSYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, szMessageTextActive);
	FillMenuEntityKeyValues(pMenuKV3, vecOrigin, angRotation, vecScales, MENUSYSTEM_INACTIVE_COLOR, MENUSYSTEM_DEFAULT_FONT_FAMILY, MENUSYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, szMessageTextInactive);

	vecKeyValues.AddToTail(pMenuKV);
	vecKeyValues.AddToTail(pMenuKV2);
	vecKeyValues.AddToTail(pMenuKV3);

	class CMenuEntityListener : public IEntityManager::IProviderAgent::IEntityListener
	{
	public:
		CMenuEntityListener(MenuSystem_Plugin *pInitPlugin)
		 :  m_pPlugin(pInitPlugin)
		{
		}

	public:
		void OnEntityCreated(CEntityInstance *pEntity, const CEntityKeyValues *pKeyValues) override
		{
			if(m_pPlugin->Logger::IsChannelEnabled(LV_DETAILED))
			{
				m_pPlugin->Logger::MessageFormat("Setting up \"%s\" menu entity\n", pEntity->GetClassname());
			}

			m_pPlugin->SettingMenuEntity(entity_upper_cast<CBaseEntity *>(pEntity));
		}

	private:
		MenuSystem_Plugin *m_pPlugin;
	} aMenuEntitySetup(this);

	SpawnEntities(vecKeyValues, pEntities, &aMenuEntitySetup);
}

void MenuSystem_Plugin::SpawnMenuEntitiesByEntity(CBaseEntity *pTarget, CUtlVector<CEntityInstance *> *pEntities)
{
	Vector vecMenuAbsOriginBackground {},
	       vecMenuAbsOrigin {};

	QAngle angMenuRotation {};

	CalculateMenuEntitiesPositionByEntity(pTarget, vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation);
	SpawnMenuEntities(vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation, pEntities);
}

// A universal way to create a second view model.
CBaseViewModel *MenuSystem_Plugin::SpawnViewModelEntity(const Vector &vecOrigin, const QAngle &angRotation, CBaseEntity *pOwner, const int nSlot)
{
	const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

	static_assert(INVALID_SPAWN_GROUP == ANY_SPAWN_GROUP);

	CEntityKeyValues *pViewModelKV = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL);

	CUtlVector<CEntityKeyValues *> vecKeyValues;

	CUtlVector<CEntityInstance *> vecEntities;

	class CViewModelEntityListener : public IEntityManager::IProviderAgent::IEntityListener
	{
	public:
		CViewModelEntityListener(MenuSystem_Plugin *pInitPlugin, CBaseEntity *pInitOwner, const int nInitSlot)
		 :  m_pPlugin(pInitPlugin),
		    m_pOwner(pInitOwner),
		    m_nSlot(nInitSlot)
		{
		}

	public:
		void OnEntityCreated(CEntityInstance *pEntity, const CEntityKeyValues *pKeyValues) override
		{
			if(m_pPlugin->Logger::IsChannelEnabled(LV_DETAILED))
			{
				m_pPlugin->Logger::MessageFormat("Setting up \"%s\" view model entity\n", pEntity->GetClassname());
			}

			m_pPlugin->SettingExtraPlayerViewModelEntity(entity_upper_cast<CBaseViewModel *>(pEntity), m_pOwner, m_nSlot);
		}

	private:
		MenuSystem_Plugin *m_pPlugin;
		CBaseEntity *m_pOwner;
		const int m_nSlot;
	} aViewModelEntitySetup(this, pOwner, nSlot);

	FillViewModelEntityKeyValues(pViewModelKV, vecOrigin, angRotation);
	vecKeyValues.AddToTail(pViewModelKV);
	SpawnEntities(vecKeyValues, &vecEntities, &aViewModelEntitySetup);

	return entity_upper_cast<CBaseViewModel *>(vecEntities[0]);
}

void MenuSystem_Plugin::TeleportMenuEntitiesToCSPlayer(CCSPlayerPawnBase *pTarget, const CUtlVector<CEntityInstance *> &vecEntities)
{
	auto &aBaseEntity = GetGameDataStorage().GetBaseEntity();

	Vector vecMenuAbsOriginBackground {},
	       vecMenuAbsOrigin {};

	QAngle angMenuRotation {};

	CalculateMenuEntitiesPositionByCSPlayer(pTarget, vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation);

	FOR_EACH_VEC(vecEntities, i)
	{
		auto *pEntity = vecEntities[i];

		aBaseEntity.Teleport(pEntity, i ? vecMenuAbsOrigin : vecMenuAbsOriginBackground, angMenuRotation);
	}
}

void MenuSystem_Plugin::AttachMenuEntitiesToEntity(CBaseEntity *pTarget, const CUtlVector<CEntityInstance *> &vecEntities)
{
	auto &aBaseEntity = GetGameDataStorage().GetBaseEntity();

	auto aParentVariant = variant_t("!activator");

	for(auto *pEntity : vecEntities)
	{
		aBaseEntity.AcceptInput(pEntity, "SetParent", pTarget, NULL, &aParentVariant, 0);
	}
}

bool MenuSystem_Plugin::AttachMenuEntitiesToCSPlayer(CCSPlayerPawnBase *pTarget, const CUtlVector<CEntityInstance *> &vecEntities)
{
	auto &aBaseEntity = GetGameDataStorage().GetBaseEntity();

	auto aViewModelServicesAccessor = CCSPlayerPawnBase_Helper::GetViewModelServicesAccessor(pTarget);

	CCSPlayer_ViewModelServices *pCSPlayerViewModelServices = aViewModelServicesAccessor;

	if(!pCSPlayerViewModelServices)
	{
		Logger::WarningFormat("Failed to get a player view model services\n");

		return false;
	}

	static const int s_nExtraViewModelSlot = 2;

	auto aParentVariant = variant_t("!activator");

	CHandle<CBaseViewModel> &hPlayerViewModel = CCSPlayer_ViewModelServices_Helper::GetViewModelAccessor(pCSPlayerViewModelServices)[s_nExtraViewModelSlot];

	CBaseViewModel *pPlayerViewModel = hPlayerViewModel.Get();

	Vector vecMenuAbsOriginBackground {},
	       vecMenuAbsOrigin {};

	QAngle angMenuRotation {};

	CalculateMenuEntitiesPositionByEntity(pTarget, vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation);

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		const auto &aConcat = g_aEmbedConcat;

		CBufferStringGrowable<256> sBuffer;

		aConcat.AppendHeadToBuffer(sBuffer, "Menu entities position");
		aConcat.AppendToBuffer(sBuffer, "Origin", vecMenuAbsOrigin);
		aConcat.AppendToBuffer(sBuffer, "Rotation", angMenuRotation);

		Logger::Detailed(sBuffer);
	}

	if(!pPlayerViewModel)
	{
		auto *pExtraPlayerViewModel = SpawnViewModelEntity(vecMenuAbsOrigin, angMenuRotation, pTarget, s_nExtraViewModelSlot);

		hPlayerViewModel.Set(pExtraPlayerViewModel);
		pPlayerViewModel = pExtraPlayerViewModel;
	}

	aBaseEntity.AcceptInput(pPlayerViewModel, "FollowEntity", pTarget, NULL, &aParentVariant, 0);

	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("Player view model: \"%s\" (%d)\n", pPlayerViewModel->GetClassname(), pPlayerViewModel->GetEntityIndex().Get());
	}

	FOR_EACH_VEC(vecEntities, i)
	{
		auto *pEntity = vecEntities[i];

		aBaseEntity.Teleport(pEntity, i ? vecMenuAbsOrigin : vecMenuAbsOriginBackground, angMenuRotation);
		aBaseEntity.AcceptInput(pEntity, "SetParent", pPlayerViewModel, NULL, &aParentVariant, 0);
	}

	aViewModelServicesAccessor.MarkNetworkChanged(); // Update CCSPlayer_ViewModelServices < CPlayer_ViewModelServices state of the view model entities.

	return true;
}

bool MenuSystem_Plugin::SettingMenuEntity(CEntityInstance *pEntity)
{
	{
		auto aEFlagsAccessor = CBaseEntity_Helper::GetEFlagsAccessor(entity_upper_cast<CBaseEntity *>(pEntity));

		aEFlagsAccessor = EF_MENU;
		aEFlagsAccessor.MarkNetworkChanged();
	}

	return true;
}

bool MenuSystem_Plugin::SettingExtraPlayerViewModelEntity(CBaseViewModel *pViewModelEntity, CBaseEntity *pOwner, const int nSlot)
{
	{
		auto aViewModelIndexAccessor = CBaseViewModel_Helper::GetViewModelIndexAccessor(pViewModelEntity);

		aViewModelIndexAccessor = nSlot;
		aViewModelIndexAccessor.MarkNetworkChanged();
	}

	{
		auto aOwnerEntityAccessor = CBaseEntity_Helper::GetOwnerEntityAccessor(pViewModelEntity);

		aOwnerEntityAccessor = pOwner;
		aOwnerEntityAccessor.MarkNetworkChanged();
	}

	{
		auto aEFlagsAccessor = CBaseEntity_Helper::GetEFlagsAccessor(pViewModelEntity);

		aEFlagsAccessor = EF_NODRAW;
		aEFlagsAccessor.MarkNetworkChanged();
	}

	return true;
}

bool MenuSystem_Plugin::RegisterGameResource(char *error, size_t maxlen)
{
	if(!RegisterGameEntitySystem(m_pEntityManagerProviderAgent->GetSystem()))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a (game) entity system", maxlen);
		}

		return false;
	}

	return true;
}

bool MenuSystem_Plugin::UnregisterGameResource(char *error, size_t maxlen)
{
	if(!UnregisterGameEntitySystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a (game) entity system", maxlen);
		}

		return false;
	}

	return true;
}

bool MenuSystem_Plugin::RegisterGameFactory(char *error, size_t maxlen)
{
	CBaseGameSystemFactory **ppFactory = GetFirstGameSystemPointer();

	if(!ppFactory)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a first game system factory", maxlen);
		}

		return false;
	}

	if(!RegisterFirstGameSystem(ppFactory))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a first game factory", maxlen);
		}

		return false;
	}

	m_pFactory = new CGameSystemStaticFactory<MenuSystem_Plugin>(GetName(), this);

	return true;
}

bool MenuSystem_Plugin::UnregisterGameFactory(char *error, size_t maxlen)
{
	if(m_pFactory)
	{
		m_pFactory->Shutdown();

		// Clean up smart dispatcher listener callbacks.
		{
			const auto *pGameSystem = m_pFactory->GetStaticGameSystem();

			auto **ppDispatcher = GetGameSystemEventDispatcherPointer();

			Assert(ppDispatcher);

			auto *pDispatcher = *ppDispatcher;

			if(pDispatcher)
			{
				auto *pfuncListeners = pDispatcher->m_funcListeners;

				Assert(pfuncListeners);

				auto &funcListeners = *pfuncListeners;

				FOR_EACH_VEC_BACK(funcListeners, i)
				{
					auto &vecListeners = funcListeners[i];

					FOR_EACH_VEC_BACK(vecListeners, j)
					{
						if(pGameSystem == vecListeners[j])
						{
							vecListeners.FastRemove(j);

							break;
						}
					}

					if(!vecListeners.Count())
					{
						funcListeners.FastRemove(i);
					}
				}
			}
		}

		m_pFactory->DestroyGameSystem(this);
		m_pFactory->Destroy();
	}

	if(!UnregisterFirstGameSystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a first game factory", maxlen);
		}

		return false;
	}

	return true;
}

bool MenuSystem_Plugin::RegisterSource2Server(char *error, size_t maxlen)
{
	IGameEventManager2 **ppGameEventManager = GetGameEventManagerPointer();

	if(!ppGameEventManager)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a game event manager", maxlen);
		}

		return false;
	}

	if(!RegisterGameEventManager(*ppGameEventManager))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a game event manager", maxlen);
		}

		return false;
	}

	return true;
}

bool MenuSystem_Plugin::UnregisterSource2Server(char *error, size_t maxlen)
{
	if(!UnregisterGameEventManager())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a game event manager", maxlen);
		}

		return false;
	}

	return true;
}

bool MenuSystem_Plugin::RegisterNetMessages(char *error, size_t maxlen)
{
	const struct
	{
		const char *pszName;
		INetworkMessageInternal **ppInternal;
	} aMessageInitializers[] =
	{
		{
			"CSVCMsg_GetCvarValue",
			&m_pGetCvarValueMessage,
		},
		{
			"CUserMessageSayText2",
			&m_pSayText2Message,
		},
		{
			"CUserMessageTextMsg",
			&m_pTextMsgMessage,
		},
		// {
		// 	"CCSUsrMsg_VGUIMenu",
		// 	&m_pVGUIMenuMessage,
		// },
	};

	for(const auto &aMessageInitializer : aMessageInitializers)
	{
		const char *pszMessageName = aMessageInitializer.pszName;

		INetworkMessageInternal *pMessage = g_pNetworkMessages->FindNetworkMessagePartial(pszMessageName);

		if(!pMessage)
		{
			if(error && maxlen)
			{
				snprintf(error, maxlen, "Failed to get \"%s\" message", pszMessageName);
			}

			return false;
		}

		*aMessageInitializer.ppInternal = pMessage;
	}

	return true;
}

bool MenuSystem_Plugin::UnregisterNetMessages(char *error, size_t maxlen)
{
	m_pSayText2Message = NULL;

	return true;
}

bool MenuSystem_Plugin::ParseLanguages(char *error, size_t maxlen)
{
	std::string sTranslationsFilesPath = m_sBaseGameDirectory + CORRECT_PATH_SEPARATOR_S MENUSYSTEM_GAME_LANGUAGES_FILES;

	const char *pszPathID = MENUSYSTEM_BASE_PATHID, 
	           *pszLanguagesFiles = sTranslationsFilesPath.c_str();

	CUtlVector<CUtlString> vecLangugesFiles;
	CUtlVector<CUtlString> vecSubmessages;

	CUtlString sMessage;

	auto aWarnings = Logger::CreateWarningsScope();

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sMessage, NULL, pszPathID}, g_KV3Format_Generic});

	g_pFullFileSystem->FindFileAbsoluteList(vecLangugesFiles, pszLanguagesFiles, pszPathID);

	if(!vecLangugesFiles.Count())
	{
		if(error && maxlen)
		{
			snprintf(error, maxlen, "No found languages by \"%s\" path", pszLanguagesFiles);
		}

		return false;
	}

	for(const auto &sFile : vecLangugesFiles)
	{
		const char *pszFilename = sFile.Get();

		AnyConfig::Anyone aLanguagesConfig;

		aLoadPresets.m_pszFilename = pszFilename;

		if(!aLanguagesConfig.Load(aLoadPresets))
		{
			aWarnings.PushFormat("\"%s\": %s", pszFilename, sMessage.Get());

			continue;
		}

		if(!ParseLanguages(aLanguagesConfig.Get(), vecSubmessages))
		{
			aWarnings.PushFormat("\"%s\"", pszFilename);

			for(const auto &sSubmessage : vecSubmessages)
			{
				aWarnings.PushFormat("\t%s", sSubmessage.Get());
			}

			continue;
		}
	}

	if(aWarnings.Count())
	{
		aWarnings.Send([&](const CUtlString &sMessage)
		{
			Logger::Warning(sMessage);
		});
	}

	return true;
}

bool MenuSystem_Plugin::ParseLanguages(KeyValues3 *pRoot, CUtlVector<CUtlString> &vecMessages)
{
	int iMemberCount = pRoot->GetMemberCount();

	if(!iMemberCount)
	{
		vecMessages.AddToTail("No members");

		return true;
	}

	const KeyValues3 *pDefaultData = pRoot->FindMember("default");

	const char *pszServerContryCode = pDefaultData ? pDefaultData->GetString() : "en";

	m_aServerLanguage.SetCountryCode(pszServerContryCode);

	for(KV3MemberId_t n = 0; n < iMemberCount; n++)
	{
		const char *pszMemberName = pRoot->GetMemberName(n);

		auto sMemberSymbol = GetLanguageSymbol(pszMemberName);

		const KeyValues3 *pMember = pRoot->GetMember(n);

		const char *pszMemberValue = pMember->GetString(pszServerContryCode);

		m_mapLanguages.Insert(sMemberSymbol, {sMemberSymbol, pszMemberValue});
	}

	return true;
}

bool MenuSystem_Plugin::ClearLanguages(char *error, size_t maxlen)
{
	m_vecLanguages.Purge();

	return true;
}

bool MenuSystem_Plugin::ParseTranslations(char *error, size_t maxlen)
{
	std::string sTranslationsFilesPath = m_sBaseGameDirectory + CORRECT_PATH_SEPARATOR_S MENUSYSTEM_GAME_TRANSLATIONS_FILES;

	const char *pszPathID = MENUSYSTEM_BASE_PATHID, 
	           *pszTranslationsFiles = sTranslationsFilesPath.c_str();

	CUtlVector<CUtlString> vecTranslationsFiles;

	Translations::CBufferStringVector vecSubmessages;

	CUtlString sMessage;

	auto aWarnings = Logger::CreateWarningsScope();

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sMessage, NULL, pszPathID}, g_KV3Format_Generic});

	g_pFullFileSystem->FindFileAbsoluteList(vecTranslationsFiles, pszTranslationsFiles, pszPathID);

	if(!vecTranslationsFiles.Count())
	{
		if(error && maxlen)
		{
			snprintf(error, maxlen, "No found translations by \"%s\" path", pszTranslationsFiles);
		}

		return false;
	}

	for(const auto &sFile : vecTranslationsFiles)
	{
		const char *pszFilename = sFile.Get();

		AnyConfig::Anyone aTranslationsConfig;

		aLoadPresets.m_pszFilename = pszFilename;

		if(!aTranslationsConfig.Load(aLoadPresets))
		{
			aWarnings.PushFormat("\"%s\": %s", pszFilename, sMessage.Get());

			continue;
		}

		if(!Translations::Parse(aTranslationsConfig.Get(), vecSubmessages))
		{
			aWarnings.PushFormat("\"%s\"", pszFilename);

			for(const auto &sSubmessage : vecSubmessages)
			{
				aWarnings.PushFormat("\t%s", sSubmessage.Get());
			}

			continue;
		}
	}

	if(aWarnings.Count())
	{
		aWarnings.Send([&](const CUtlString &sMessage)
		{
			Logger::Warning(sMessage);
		});
	}

	return true;
}

bool MenuSystem_Plugin::ClearTranslations(char *error, size_t maxlen)
{
	Translations::Purge();

	return true;
}


bool MenuSystem_Plugin::HookGameEvents(char *error, size_t maxlen)
{
	if(!Menu::GameEventSystem::HookAll())
	{
		strncpy(error, "Failed to hook game events", maxlen);

		return false;
	}

	return true;
}

bool MenuSystem_Plugin::UnhookGameEvents(char *error, size_t maxlen)
{
	if(!Menu::GameEventSystem::UnhookAll())
	{
		strncpy(error, "Failed to unhook game events", maxlen);

		return false;
	}

	return true;
}

void MenuSystem_Plugin::OnReloadGameDataCommand(const CCommandContext &context, const CCommand &args)
{
	char error[256];

	if(!LoadProvider(error, sizeof(error)))
	{
		Logger::WarningFormat("%s\n", error);
	}
}

void MenuSystem_Plugin::OnReloadSchemaCommand(const CCommandContext &context, const CCommand &args)
{
	char error[256];

	if(!LoadSchema(error, sizeof(error)))
	{
		Logger::WarningFormat("%s\n", error);
	}
}

void MenuSystem_Plugin::OnMenuSelectCommand(const CCommandContext &context, const CCommand &args)
{
	Logger::MessageFormat("Menu select: slot %d!!!\n", args.ArgC() > 1 ? atoi(args.Arg(1)) : -1);
}

void MenuSystem_Plugin::OnDispatchConCommandHook(ConCommandHandle hCommand, const CCommandContext &aContext, const CCommand &aArgs)
{
	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(%d, %d, %s)\n", __FUNCTION__, hCommand.GetIndex(), aContext.GetPlayerSlot().Get(), aArgs.GetCommandString());
	}

	auto aPlayerSlot = aContext.GetPlayerSlot();

	const char *pszArg0 = aArgs.Arg(0);

	static const char szSayCommand[] = "say";

	size_t nSayNullTerminated = sizeof(szSayCommand) - 1;

	if(!V_strncmp(pszArg0, (const char *)szSayCommand, nSayNullTerminated))
	{
		if(!pszArg0[nSayNullTerminated] || !V_strcmp(&pszArg0[nSayNullTerminated], "_team"))
		{
			const char *pszArg1 = aArgs.Arg(1);

			// Skip spaces.
			while(*pszArg1 == ' ')
			{
				pszArg1++;
			}

			bool bIsSilent = *pszArg1 == Menu::ChatCommandSystem::GetSilentTrigger();

			if(bIsSilent || *pszArg1 == Menu::ChatCommandSystem::GetPublicTrigger())
			{
				pszArg1++; // Skip a command character.

				// Print a chat message before.
				if(!bIsSilent && g_pCVar)
				{
					SH_CALL(g_pCVar, &ICvar::DispatchConCommand)(hCommand, aContext, aArgs);
				}

				// Call the handler.
				{
					size_t nArg1Length = 0;

					// Get a length to a first space.
					while(pszArg1[nArg1Length] && pszArg1[nArg1Length] != ' ')
					{
						nArg1Length++;
					}

					CUtlVector<CUtlString> vecArgs;

					V_SplitString(pszArg1, " ", vecArgs);

					for(auto &sArg : vecArgs)
					{
						sArg.Trim(' ');
					}

					if(Logger::IsChannelEnabled(LV_DETAILED))
					{
						const auto &aConcat = g_aEmbedConcat, 
						           &aConcat2 = g_aEmbed2Concat;

						CBufferStringGrowable<1024> sBuffer;

						aConcat.AppendHeadToBuffer(sBuffer, "Handle a chat command");
						aConcat.AppendToBuffer(sBuffer, "Player slot", aPlayerSlot.Get());
						aConcat.AppendToBuffer(sBuffer, "Is silent", bIsSilent);
						aConcat.AppendToBuffer(sBuffer, "Arguments");

						for(const auto &sArg : vecArgs)
						{
							const char *pszMessageConcat[] = {aConcat2.GetStartWith(), "\"", sArg.Get(), "\"", aConcat2.GetEnd()};

							sBuffer.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
						}

						Logger::Detailed(sBuffer);
					}

					Menu::ChatCommandSystem::Handle(vecArgs[0], aPlayerSlot, bIsSilent, vecArgs);
				}

				RETURN_META(MRES_SUPERCEDE);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}

void MenuSystem_Plugin::OnStartupServerHook(const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char *)
{
	auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

	OnStartupServer(pNetServer, config, pWorldSession);

	RETURN_META(MRES_IGNORED);
}

CServerSideClientBase *MenuSystem_Plugin::OnConnectClientHook(const char *pszName, ns_address *pAddr, void *pNetInfo, C2S_CONNECT_Message *pConnectMsg, 
                                                             const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	auto *pNetServer = META_IFACEPTR(CNetworkGameServerBase);

	auto *pClient = META_RESULT_ORIG_RET(CServerSideClientBase *);

	OnConnectClient(pNetServer, pClient, pszName, pAddr, pNetInfo, pConnectMsg, pszChallenge, pAuthTicket, nAuthTicketLength, bIsLowViolence);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bool MenuSystem_Plugin::OnProcessRespondCvarValueHook(const CCLCMsg_RespondCvarValue_t &aMessage)
{
	auto *pClient = META_IFACEPTR(CServerSideClientBase);

	OnProcessRespondCvarValue(pClient, aMessage);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void MenuSystem_Plugin::OnDisconectClientHook(ENetworkDisconnectionReason eReason)
{
	auto *pClient = META_IFACEPTR(CServerSideClientBase);

	OnDisconectClient(pClient, eReason);

	RETURN_META(MRES_IGNORED);
}

void MenuSystem_Plugin::SendCvarValueQuery(IRecipientFilter *pFilter, const char *pszName, int iCookie)
{
	auto *pGetCvarValueMessage = m_pGetCvarValueMessage;

	auto *pMessage = pGetCvarValueMessage->AllocateMessage()->ToPB<CSVCMsg_GetCvarValue>();

	pMessage->set_cvar_name(pszName);
	pMessage->set_cookie(iCookie);

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pGetCvarValueMessage, pMessage, 0);

	delete pMessage;
}

void MenuSystem_Plugin::SendChatMessage(IRecipientFilter *pFilter, int iEntityIndex, bool bIsChat, const char *pszChatMessageFormat, const char *pszParam1, const char *pszParam2, const char *pszParam3, const char *pszParam4)
{
	auto *pSayText2Message = m_pSayText2Message;

	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = g_aEmbedConcat;

		CBufferStringGrowable<256> sHead;

		sHead.Insert(0, "Send chat message (");
		sHead.Insert(sHead.Length(), pSayText2Message->GetUnscopedName());
		sHead.Insert(sHead.Length(), ")");

		CBufferStringGrowable<1024> sBuffer;

		aConcat.AppendHeadToBuffer(sBuffer, sHead.Get());
		aConcat.AppendToBuffer(sBuffer, "Entity index", iEntityIndex);
		aConcat.AppendToBuffer(sBuffer, "Is chat", bIsChat);
		aConcat.AppendStringToBuffer(sBuffer, "Chat message", pszChatMessageFormat);

		if(pszParam1 && *pszParam1)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #1", pszParam1);
		}

		if(pszParam2 && *pszParam2)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #2", pszParam2);
		}

		if(pszParam3 && *pszParam3)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #3", pszParam3);
		}

		if(pszParam4 && *pszParam4)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #4", pszParam4);
		}

		Logger::Detailed(sBuffer);
	}

	auto *pMessage = pSayText2Message->AllocateMessage()->ToPB<CUserMessageSayText2>();

	pMessage->set_entityindex(iEntityIndex);
	pMessage->set_chat(bIsChat);
	pMessage->set_messagename(pszChatMessageFormat);
	pMessage->set_param1(pszParam1);
	pMessage->set_param2(pszParam2);
	pMessage->set_param3(pszParam3);
	pMessage->set_param4(pszParam4);

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pSayText2Message, pMessage, 0);

	delete pMessage;
}

void MenuSystem_Plugin::SendTextMessage(IRecipientFilter *pFilter, int iDestination, size_t nParamCount, const char *pszParam, ...)
{
	auto *pTextMsg = m_pTextMsgMessage;

	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = g_aEmbedConcat;

		CBufferStringGrowable<256> sHead;

		sHead.Insert(0, "Send text message (");
		sHead.Insert(sHead.Length(), pTextMsg->GetUnscopedName());
		sHead.Insert(sHead.Length(), ")");

		CBufferStringGrowable<1024> sBuffer;

		aConcat.AppendHeadToBuffer(sBuffer, sHead.Get());
		aConcat.AppendToBuffer(sBuffer, "Destination", iDestination);
		aConcat.AppendToBuffer(sBuffer, "Parameter", pszParam);
		Logger::Detailed(sBuffer);
	}

	auto *pMessage = pTextMsg->AllocateMessage()->ToPB<CUserMessageTextMsg>();

	pMessage->set_dest(iDestination);
	pMessage->add_param(pszParam);
	nParamCount--;

	// Parse incoming parameters.
	if(0 < nParamCount)
	{
		va_list aParams;

		va_start(aParams, pszParam);

		size_t n = 0;

		do
		{
			pMessage->add_param(va_arg(aParams, const char *));

			n++;
		}
		while(n < nParamCount);

		va_end(aParams);
	}

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pTextMsg, pMessage, 0);

	delete pMessage;
}

// void MenuSystem_Plugin::SendVGUIMenuMessage(IRecipientFilter *pFilter, const char *pszName, const bool *pIsShow, KeyValues3 *pKeys)
// {
// 	auto *pVGUIMenuMsg = m_pVGUIMenuMessage;

// 	if(Logger::IsChannelEnabled(LV_DETAILED))
// 	{
// 		const auto &aConcat = g_aEmbedConcat, 
// 		           &aConcat2 = g_aEmbed2Concat;

// 		CBufferStringGrowable<1024> sBuffer;

// 		sBuffer.Format("Send message (%s):\n", pVGUIMenuMsg->GetUnscopedName());
// 		aConcat.AppendToBuffer(sBuffer, "Name", pszName ? pszName : "<none>");

// 		if(pIsShow)
// 		{
// 			aConcat.AppendToBuffer(sBuffer, "Show", *pIsShow);
// 		}
// 		else
// 		{
// 			aConcat.AppendToBuffer(sBuffer, "Show", "<none>");
// 		}

// 		if(pKeys)
// 		{
// 			int iMemberCount = pKeys->GetMemberCount();

// 			if(iMemberCount)
// 			{
// 				aConcat.AppendToBuffer(sBuffer, "Keys");

// 				KV3MemberId_t i = 0;

// 				do
// 				{
// 					KeyValues3 *pMember = pKeys->GetMember(i);
// 					aConcat2.AppendToBuffer(sBuffer, "name", pKeys->GetMemberName(i));
// 					aConcat2.AppendToBuffer(sBuffer, "value", pMember->GetString());

// 					i++;
// 				}
// 				while(i < iMemberCount);
// 			}
// 			else
// 			{
// 				aConcat.AppendToBuffer(sBuffer, "Keys", "<no members>");
// 			}
// 		}
// 		else
// 		{
// 			aConcat.AppendToBuffer(sBuffer, "Keys", "<none>");
// 		}

// 		Logger::Detailed(sBuffer);
// 	}

// 	auto *pMessage = pVGUIMenuMsg->AllocateMessage()->ToPB<CCSUsrMsg_VGUIMenu>();

// 	if(pszName)
// 	{
// 		pMessage->set_name(pszName);
// 	}

// 	if(pIsShow)
// 	{
// 		pMessage->set_show(*pIsShow);
// 	}

// 	if(pKeys)
// 	{
// 		int iMemberCount = pKeys->GetMemberCount();

// 		if(iMemberCount)
// 		{
// 			KV3MemberId_t i = 0;

// 			do
// 			{
// 				KeyValues3 *pMember = pKeys->GetMember(i);

// 				auto *pMessageKeys = pMessage->add_keys();

// 				pMessageKeys->set_name(pKeys->GetMemberName(i));
// 				pMessageKeys->set_value(pMember->GetString());

// 				i++;
// 			}
// 			while(i < iMemberCount);
// 		}
// 	}

// 	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pVGUIMenuMsg, pMessage, 0);

// 	delete pMessage;
// }

void MenuSystem_Plugin::OnStartupServer(CNetworkGameServerBase *pNetServer, const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession)
{
	SH_ADD_HOOK_MEMFUNC(CNetworkGameServerBase, ConnectClient, pNetServer, this, &MenuSystem_Plugin::OnConnectClientHook, true);

	{
		char sMessage[256];

		bool (MenuSystem_Plugin::*pfnIntializers[])(char *error, size_t maxlen) = 
		{
			&MenuSystem_Plugin::RegisterSource2Server,
			&MenuSystem_Plugin::RegisterNetMessages,
			&MenuSystem_Plugin::HookGameEvents,
		};

		for(const auto &aInitializer : pfnIntializers)
		{
			if(!(this->*(aInitializer))(sMessage, sizeof(sMessage)))
			{
				Logger::WarningFormat("%s\n", sMessage);
			}
		}
	}
}

void MenuSystem_Plugin::OnConnectClient(CNetworkGameServerBase *pNetServer, CServerSideClientBase *pClient, const char *pszName, ns_address *pAddr, void *pNetInfo, C2S_CONNECT_Message *pConnectMsg, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	if(pClient)
	{
		SH_ADD_HOOK_MEMFUNC(CServerSideClientBase, PerformDisconnection, pClient, this, &MenuSystem_Plugin::OnDisconectClientHook, false);

		if(pClient->IsFakeClient())
		{
			return;
		}

		SH_ADD_HOOK_MEMFUNC(CServerSideClientBase, ProcessRespondCvarValue, pClient, this, &MenuSystem_Plugin::OnProcessRespondCvarValueHook, false);
	}
	else
	{
		AssertMsg(0, "Failed to get a server side client pointer");

		return;
	}

	auto *pPlayer = reinterpret_cast<CServerSideClient *>(pClient);

	auto aSlot = pClient->GetPlayerSlot();

	auto &aPlayer = GetPlayerData(aSlot);

	// Get "cl_language" cvar value from a client.
	{

		CSingleRecipientFilter aFilter(aSlot);

		const char *pszCvarName = MENUSYSTEM_CLIENT_CVAR_NAME_LANGUAGE;

		int iCookie {};

		{
			auto sConVarSymbol = GetConVarSymbol(pszCvarName);

			auto iFound = m_mapConVarCookies.Find(sConVarSymbol);

			if(m_mapConVarCookies.IsValidIndex(iFound))
			{
				auto &iFoundCookie = m_mapConVarCookies.Element(iFound);

				iFoundCookie++;
				iCookie = iFoundCookie;
			}
			else
			{
				iCookie = 0;
				m_mapConVarCookies.Insert(sConVarSymbol, iCookie);
			}
		}

		SendCvarValueQuery(&aFilter, pszCvarName, iCookie);
	}

	aPlayer.OnConnected(pPlayer);
}

bool MenuSystem_Plugin::OnProcessRespondCvarValue(CServerSideClientBase *pClient, const CCLCMsg_RespondCvarValue_t &aMessage)
{
	auto sFoundSymbol = FindConVarSymbol(aMessage.name().c_str());

	if(!sFoundSymbol.IsValid())
	{
		return false;
	}

	auto iFound = m_mapConVarCookies.Find(sFoundSymbol);

	if(!m_mapConVarCookies.IsValidIndex(iFound))
	{
		return false;
	}

	const auto &itCookie = m_mapConVarCookies.Element(iFound);

	if(itCookie != aMessage.cookie())
	{
		return false;
	}

	auto iLanguageFound = m_mapLanguages.Find(FindLanguageSymbol(aMessage.value().c_str()));

	if(!m_mapLanguages.IsValidIndex(iLanguageFound))
	{
		return false;
	}

	auto aSlot = pClient->GetPlayerSlot();

	auto &aPlayer = GetPlayerData(aSlot);

	auto &itLanguage = m_mapLanguages.Element(iLanguageFound);

	aPlayer.OnLanguageChanged(aSlot, &itLanguage);

	{
		CUtlVector<CUtlString> vecMessages;

		auto aWarnings = Logger::CreateWarningsScope();

		aPlayer.TranslatePhrases(this, this->m_aServerLanguage, vecMessages);

		for(const auto &sMessage : vecMessages)
		{
			aWarnings.Push(sMessage.Get());
		}

		aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
		{
			Logger::Warning(rgba, sContext);
		});
	}

	return true;
}

void MenuSystem_Plugin::OnDisconectClient(CServerSideClientBase *pClient, ENetworkDisconnectionReason eReason)
{
	SH_REMOVE_HOOK_MEMFUNC(CServerSideClientBase, PerformDisconnection, pClient, this, &MenuSystem_Plugin::OnDisconectClientHook, false);

	if(pClient->IsFakeClient())
	{
		return;
	}

	SH_REMOVE_HOOK_MEMFUNC(CServerSideClientBase, ProcessRespondCvarValue, pClient, this, &MenuSystem_Plugin::OnProcessRespondCvarValueHook, false);

	auto *pPlayer = reinterpret_cast<CServerSideClient *>(pClient);

	auto aSlot = pClient->GetPlayerSlot();

	auto &aPlayer = GetPlayerData(aSlot);

	aPlayer.OnDisconnected(pPlayer, eReason);
}

CUtlSymbolLarge MenuSystem_Plugin::GetConVarSymbol(const char *pszName)
{
	return m_tableConVars.AddString(pszName);
}

CUtlSymbolLarge MenuSystem_Plugin::FindConVarSymbol(const char *pszName) const
{
	return m_tableConVars.Find(pszName);
}

CUtlSymbolLarge MenuSystem_Plugin::GetLanguageSymbol(const char *pszName)
{
	return m_tableLanguages.AddString(pszName);
}

CUtlSymbolLarge MenuSystem_Plugin::FindLanguageSymbol(const char *pszName) const
{
	return m_tableLanguages.Find(pszName);
}
