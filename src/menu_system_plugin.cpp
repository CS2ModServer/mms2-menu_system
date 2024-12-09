/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Menu System
 * Written by komashchenko & Wend4r (Borys Komashchenko & Vladimir Ezhikov).
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

#include <menu_system_plugin.hpp>
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
#include <usermessages.pb.h>
// #include <cstrike15_usermessages.pb.h>

#define EF_MENU EF_BONEMERGE | EF_BRIGHTLIGHT | EF_DIMLIGHT | EF_NOINTERP | EF_NOSHADOW | EF_NODRAW | EF_NORECEIVESHADOW | EF_BONEMERGE_FASTCULL | EF_ITEM_BLINK | EF_PARENT_ANIMATES

using CBaseEntity_Helper = MenuSystem::Schema::CBaseEntity_Helper;
using CBaseModelEntity_Helper = MenuSystem::Schema::CBaseModelEntity_Helper;
using CBasePlayerController_Helper = MenuSystem::Schema::CBasePlayerController_Helper;
using CBaseViewModel_Helper = MenuSystem::Schema::CBaseViewModel_Helper;
using CBodyComponent_Helper = MenuSystem::Schema::CBodyComponent_Helper;
using CCSPlayerPawnBase_Helper = MenuSystem::Schema::CCSPlayerPawnBase_Helper;
using CGameSceneNode_Helper = MenuSystem::Schema::CGameSceneNode_Helper;
using CCSPlayer_ViewModelServices_Helper = MenuSystem::Schema::CCSPlayer_ViewModelServices_Helper;

SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandHandle, const CCommandContext &, const CCommand &);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t &, ISource2WorldSession *, const char *);
SH_DECL_HOOK8(CNetworkGameServerBase, ConnectClient, SH_NOATTRIB, 0, CServerSideClientBase *, const char *, ns_address *, int, CCLCMsg_SplitPlayerConnect_t *, const char *, const byte *, int, bool);
SH_DECL_HOOK1(CServerSideClientBase, ProcessRespondCvarValue, SH_NOATTRIB, 0, bool, const CCLCMsg_RespondCvarValue_t &);
SH_DECL_HOOK1_void(CServerSideClientBase, PerformDisconnection, SH_NOATTRIB, 0, ENetworkDisconnectionReason);

static MenuSystemPlugin s_aMenuSystemPlugin;
MenuSystemPlugin *g_pMenuSystemPlugin = &s_aMenuSystemPlugin;

static IEntityManager *s_pEntityManager = nullptr;
static IEntityManager *s_pEntityManagerProviderAgent = nullptr;
static IEntityManager *s_pEntityManagerSpawnGroupMgrProvider = nullptr;

const ConcatLineString s_aEmbedConcat =
{
	{
		"\t", // Start message.
		": ", // Padding of key & value.
		"\n", // End.
		"\n\t", // End and next line.
	}
};

const ConcatLineString s_aEmbed2Concat =
{
	{
		"\t\t",
		": ",
		"\n",
		"\n\t\t",
	}
};

PLUGIN_EXPOSE(MenuSystemPlugin, s_aMenuSystemPlugin);

MenuSystemPlugin::MenuSystemPlugin()
 :  Logger(GetName(), [](LoggingChannelID_t nTagChannelID)
    {
    	LoggingSystem_AddTagToChannel(nTagChannelID, s_aMenuSystemPlugin.GetLogTag());
    }, 0, LV_DETAILED, MENU_SYSTEM_LOGGINING_COLOR),

    CBaseEntity_Helper(this),
    CBaseModelEntity_Helper(this),
    CBasePlayerController_Helper(this),
    CBaseViewModel_Helper(this),
    CBodyComponent_Helper(this),
    CCSPlayerPawnBase_Helper(this),
    CGameSceneNode_Helper(this),
    CCSPlayer_ViewModelServices_Helper(this),

    m_aEnableGameEventsDetaillsConVar("mm_" META_PLUGIN_PREFIX "_enable_game_events_details", FCVAR_RELEASE | FCVAR_GAMEDLL, "Enable detail messages of game events", false, true, false, true, true),
    m_mapConVarCookies(DefLessFunc(const CUtlSymbolLarge)),
    m_mapLanguages(DefLessFunc(const CUtlSymbolLarge))
{
}

bool MenuSystemPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
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

		DumpGlobals(s_aEmbedConcat, sMessage);
		Logger::Detailed(sMessage);
	}

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

	if(!InitEntityManager(error, maxlen))
	{
		return false;
	}

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		CBufferStringGrowable<1024> sMessage;

		DumpEntityManager(s_aEmbedConcat, sMessage);
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

	// See "FireGameEvent" method.
	{
		// m_vecGameEvents.AddToTail("round_start"); // Hook Round Start event to respawn menu entities.
		m_vecGameEvents.AddToTail("player_team"); // Hook Player Team to change the menu entities way to display.
	}

	SH_ADD_HOOK(ICvar, DispatchConCommand, g_pCVar, SH_MEMBER(this, &MenuSystemPlugin::OnDispatchConCommandHook), false);
	SH_ADD_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &MenuSystemPlugin::OnStartupServerHook, true);

	// Register chat commands.
	MenuSystem::ChatCommandSystem::Register("menu", [&](CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArguments)
	{
		CSingleRecipientFilter aFilter(aSlot);

		int iClient = aSlot.Get();

		Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

		const auto &aPlayer = m_aPlayers[iClient];

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
			auto *pPlayerController = reinterpret_cast<CBasePlayerController *>(g_pEntitySystem->GetEntityInstance(CEntityIndex(iClient + 1)));

			if(pPlayerController)
			{
				CCSPlayerPawnBase *pCSPlayerPawn = CBasePlayerController_Helper::GetPawnAccessor(pPlayerController)->Get();

				if(pCSPlayerPawn)
				{
					CUtlVector<CEntityInstance *> vecEntitites;

					SpawnMenuEntitiesByEntity(pCSPlayerPawn, &vecEntitites);
					AttachMenuEntitiesToCSPlayer(pCSPlayerPawn, vecEntitites);
					// m_mapPlayerEntities.Insert(iClient, vecEntitites);
				}
				else
				{
					Logger::WarningFormat("Failed to get player pawn. Client index is %d\n", iClient);
				}
			}
			else
			{
				Logger::WarningFormat("Failed to get player entity. Client index is %d\n", iClient);
			}
		}
	});

	MenuSystem::ChatCommandSystem::Register("menu_clear", [&](CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArguments)
	{
		// const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

		// FOR_EACH_MAP_FAST(m_mapPlayerEntities, i)
		// {
		// 	const auto iClient = m_mapPlayerEntities.Key(i);

		// 	for(auto *pEntity : m_mapPlayerEntities.Element(i))
		// 	{
		// 		m_pEntityManagerProviderAgent->PushDestroyQueue(pEntity);
		// 	}
		// }

		// m_pEntityManagerProviderAgent->ExecuteDestroyQueued();
		// m_mapPlayerEntities.Purge();
	});

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
					OnConnectClient(pNetServer, pClient, pClient->GetClientName(), &pClient->m_nAddr, -1, NULL, NULL, NULL, 0, pClient->m_bLowViolence);
				}
			}
		}

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
			if(!LoadMenuSpawnGroups())
			{
				Logger::Warning("Failed to load the menu spawn groups\n");
			}
		}
	}

	ismm->AddListener(static_cast<ISmmPlugin *>(this), static_cast<IMetamodListener *>(this));

	Logger::MessageFormat("%s started!\n", GetName());

	return true;
}

bool MenuSystemPlugin::Unload(char *error, size_t maxlen)
{
	{
		auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

		if(pNetServer)
		{
			SH_REMOVE_HOOK_MEMFUNC(CNetworkGameServerBase, ConnectClient, pNetServer, this, &MenuSystemPlugin::OnConnectClientHook, true);
		}
	}

	SH_REMOVE_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &MenuSystemPlugin::OnStartupServerHook, true);

	Assert(UnhookGameEvents());

	Assert(ClearGameEvents());
	Assert(ClearLanguages());
	Assert(ClearTranslations());

	if(!UnloadProvider(error, maxlen))
	{
		return false;
	}

	if(!UnloadSchema(error, maxlen))
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

bool MenuSystemPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool MenuSystemPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

void MenuSystemPlugin::AllPluginsLoaded()
{
	/**
	 * AMNOTE: This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

const char *MenuSystemPlugin::GetAuthor()        { return META_PLUGIN_AUTHOR; }
const char *MenuSystemPlugin::GetName()          { return META_PLUGIN_NAME; }
const char *MenuSystemPlugin::GetDescription()   { return META_PLUGIN_DESCRIPTION; }
const char *MenuSystemPlugin::GetURL()           { return META_PLUGIN_URL; }
const char *MenuSystemPlugin::GetLicense()       { return META_PLUGIN_LICENSE; }
const char *MenuSystemPlugin::GetVersion()       { return META_PLUGIN_VERSION; }
const char *MenuSystemPlugin::GetDate()          { return META_PLUGIN_DATE; }
const char *MenuSystemPlugin::GetLogTag()        { return META_PLUGIN_LOG_TAG; }

void *MenuSystemPlugin::OnMetamodQuery(const char *iface, int *ret)
{
	if(!strcmp(iface, MENU_SYSTEM_INTERFACE_NAME))
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

CGameEntitySystem **MenuSystemPlugin::GetGameEntitySystemPointer() const
{
	return reinterpret_cast<CGameEntitySystem **>((uintptr_t)g_pGameResourceServiceServer + GetGameDataStorage().GetGameResource().GetEntitySystemOffset());
}

CBaseGameSystemFactory **MenuSystemPlugin::GetFirstGameSystemPointer() const
{
	return GetGameDataStorage().GetGameSystem().GetFirstPointer();
}

IGameEventManager2 **MenuSystemPlugin::GetGameEventManagerPointer() const
{
	return reinterpret_cast<IGameEventManager2 **>(GetGameDataStorage().GetSource2Server().GetGameEventManagerPointer());
}

const IMenuSystem::ILanguage *MenuSystemPlugin::GetLanguageByName(const char *psz) const
{
	auto iFound = m_mapLanguages.Find(FindLanguageSymbol(psz));

	return m_mapLanguages.IsValidIndex(iFound) ? &m_mapLanguages.Element(iFound) : nullptr;
}

IMenuSystem::IPlayer *MenuSystemPlugin::GetPlayer(const CPlayerSlot &aSlot)
{
	return &GetPlayerData(aSlot);
}

MenuSystemPlugin::CPlayer &MenuSystemPlugin::GetPlayerData(const CPlayerSlot &aSlot)
{
	int iClient = aSlot.Get();

	Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

	return m_aPlayers[iClient];
}

bool MenuSystemPlugin::Init()
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}

	return true;
}

void MenuSystemPlugin::PostInit()
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}
}

void MenuSystemPlugin::Shutdown()
{
	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}
}

GS_EVENT_MEMBER(MenuSystemPlugin, GameActivate)
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
		if(!LoadMenuSpawnGroups())
		{
			Logger::Warning("Failed to load the menu spawn groups\n");
		}
	}
}

GS_EVENT_MEMBER(MenuSystemPlugin, GameDeactivate)
{
	// ...
}

GS_EVENT_MEMBER(MenuSystemPlugin, ServerPostEntityThink)
{
	// FOR_EACH_MAP_FAST(m_mapPlayerEntities, i)
	// {
	// 	const auto iClient = m_mapPlayerEntities.Key(i);

	// 	auto *pPlayer = g_pEntitySystem->GetEntityInstance(CEntityIndex(iClient + 1));

	// 	if(pPlayer)
	// 	{
	// 		auto *pPlayerPawn = CBasePlayerController_Helper::GetPawn(reinterpret_cast<CBasePlayerController *>(pPlayer))->Get();

	// 		if(pPlayerPawn)
	// 		{
	// 			TeleportMenuEntitiesToCSPlayer(pPlayerPawn, m_mapPlayerEntities.Element(i));
	// 		}
	// 	}
	// }
}

void MenuSystemPlugin::FireGameEvent(IGameEvent *event)
{
	if(m_aEnableGameEventsDetaillsConVar.GetValue())
	{
		KeyValues3 *pEventDataKeys = event->GetDataKeys();

		if(!pEventDataKeys)
		{
			Logger::WarningFormat("Data keys is empty at \"%s\" event\n", event->GetName());

			return;
		}

		if(Logger::IsChannelEnabled(LS_DETAILED))
		{
			int iMemberCount = pEventDataKeys->GetMemberCount();

			if(!iMemberCount)
			{
				Logger::WarningFormat("No members at \"%s\" event\n", event->GetName());

				return;
			}

			{
				auto aDetails = Logger::CreateDetailsScope();

				aDetails.PushFormat("\"%s\":", event->GetName());
				aDetails.Push("{");

				for(KV3MemberId_t id = 0; id < iMemberCount; id++)
				{
					const char *pEventMemberName = pEventDataKeys->GetMemberName(id);

					KeyValues3 *pEventMember = pEventDataKeys->GetMember(id);

					CBufferStringGrowable<128> sEventMember;

					pEventMember->ToString(sEventMember, KV3_TO_STRING_DONT_CLEAR_BUFF);
					aDetails.PushFormat("\t\"%s\":\t%s", pEventMemberName, sEventMember.Get());
				}

				aDetails.Push("}");
				aDetails.Send([&](const CUtlString &sMessage)
				{
					Logger::Detailed(sMessage);
				});
			}
		}
	}

	// "round_start"
	{
		// SpawnMenuEntities();
		// LoadMenuSpawnGroups();
	}

	// "player_team"
	{
		if(event->GetInt("team") <= TEAM_SPECTATOR)
		{
			int iClient = event->GetPawnEntityIndex("userid").Get() - 1;


		}
	}
}

void MenuSystemPlugin::OnSpawnGroupAllocated(SpawnGroupHandle_t hSpawnGroup, ISpawnGroup *pSpawnGroup)
{
	if(Logger::Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(hSpawnGroup = %d, pSpawnGroup = %p)\n", __FUNCTION__, hSpawnGroup, pSpawnGroup);
	}

	// AsyncSpawnMenuEntities();
}

void MenuSystemPlugin::OnSpawnGroupInit(SpawnGroupHandle_t hSpawnGroup, IEntityResourceManifest *pManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry)
{
	if(Logger::Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(hSpawnGroup = %d, pManifest = %p, pConfig = %p, pRegistry = %p)\n", __FUNCTION__, hSpawnGroup, pManifest, pConfig, pRegistry);
	}

	Assert(pManifest);

	m_pEntityManagerProviderAgent->AddResourceToEntityManifest(pManifest, "materials/dev/annotation_worldtext_background.vmat");
	m_pEntityManagerProviderAgent->AddResourceToEntityManifest(pManifest, "materials/editor/icon_empty.vmat");
}

void MenuSystemPlugin::OnSpawnGroupCreateLoading(SpawnGroupHandle_t hSpawnGroup, CMapSpawnGroup *pMapSpawnGroup, bool bSynchronouslySpawnEntities, bool bConfirmResourcesLoaded, CUtlVector<const CEntityKeyValues *> &vecKeyValues)
{
	if(Logger::Logger::IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(hSpawnGroup = %d, pMapSpawnGroup = %p, bSynchronouslySpawnEntities = %s, bConfirmResourcesLoaded = %s, &vecKeyValues = %p)\n", __FUNCTION__, hSpawnGroup, pMapSpawnGroup, bSynchronouslySpawnEntities ? "true" : "false", bConfirmResourcesLoaded ? "true" : "false", &vecKeyValues);
	}

	const Vector vecBackgroundOrigin = {-42.0f, 30.0f, -159.875f}, 
	             vecOrigin = {-42.0f, 30.0f, -160.0f};

	const QAngle angRotation = {180.0f, 0.0f, 0.0f};

	const Vector vecScales = {1.f, 1.f, 1.f};

	CEntityKeyValues *pMenuKV = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL),
	                 *pMenuKV2 = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL), 
	                 *pMenuKV3 = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL);

	FillMenuEntityKeyValues(pMenuKV, vecBackgroundOrigin, angRotation, vecScales, MENU_SYSTEM_BACKGROUND_COLOR, MENU_SYSTEM_DEFAULT_FONT_FAMILY, MENU_SYSTEM_BACKGROUND_MATERIAL_NAME, "Title\n\n1. Active");
	FillMenuEntityKeyValues(pMenuKV2, vecOrigin, angRotation, vecScales, MENU_SYSTEM_ACTIVE_COLOR, MENU_SYSTEM_DEFAULT_FONT_FAMILY, MENU_SYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, "\n\n1 Active");
	FillMenuEntityKeyValues(pMenuKV3, vecOrigin, angRotation, vecScales, MENU_SYSTEM_INACTIVE_COLOR, MENU_SYSTEM_DEFAULT_FONT_FAMILY, MENU_SYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, "Title");

	g_pEntitySystem->AddRefKeyValues(pMenuKV);
	g_pEntitySystem->AddRefKeyValues(pMenuKV2);
	g_pEntitySystem->AddRefKeyValues(pMenuKV3);

	vecKeyValues.AddToTail(pMenuKV);
	vecKeyValues.AddToTail(pMenuKV2);
	vecKeyValues.AddToTail(pMenuKV3);
}

void MenuSystemPlugin::OnSpawnGroupDestroyed(SpawnGroupHandle_t hSpawnGroup)
{
	m_pMySpawnGroupInstance->RemoveNotificationsListener(static_cast<IEntityManager::IProviderAgent::ISpawnGroupNotifications *>(this));
}

bool MenuSystemPlugin::InitProvider(char *error, size_t maxlen)
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

bool MenuSystemPlugin::LoadProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Load(MENU_SYSTEM_BASE_DIR, MENU_SYSTEM_BASE_PATHID, vecMessages);

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

bool MenuSystemPlugin::UnloadProvider(char *error, size_t maxlen)
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

bool MenuSystemPlugin::InitSchema(char *error, size_t maxlen)
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

	GameData::CBufferStringVector vecMessages;

	bool bResult = CSystem::Init(g_pSchemaSystem, vecLoadLibraries, &vecMessages);

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
			strncpy(error, "Failed to load schema helper. See warnings", maxlen);
		}
	}

	return bResult;
}

bool MenuSystemPlugin::UnloadSchema(char *error, size_t maxlen)
{
	CSystem::Destroy();

	return true;
}

bool MenuSystemPlugin::InitEntityManager(char *error, size_t maxlen)
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
		m_pEntityManagerSpawnGroupProvider = m_pEntityManager->GetSpawnGroupManager();

		if(!m_pEntityManagerSpawnGroupProvider)
		{
			strncpy(error, "Failed to get a entity manager spawn group mgr interface", maxlen);

			return false;
		}
	}

	return true;
}

void MenuSystemPlugin::DumpEntityManager(const ConcatLineString &aConcat, CBufferString &sOutput)
{
	GLOBALS_APPEND_VARIABLE(m_pEntityManager);
	GLOBALS_APPEND_VARIABLE(m_pEntityManagerProviderAgent);
	GLOBALS_APPEND_VARIABLE(m_pEntityManagerSpawnGroupProvider);
}

bool MenuSystemPlugin::UnloadEntityManager(char *error, size_t maxlen)
{
	return true;
}

bool MenuSystemPlugin::LoadMenuSpawnGroups(const Vector &aWorldOrigin)
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

		if(pSpawnGroupInstance->Load(aDesc, aWorldOrigin))
		{
			m_pMySpawnGroupInstance = pSpawnGroupInstance;
		}
		else
		{
			Logger::WarningFormat("Failed to load \"%s\" spawn group", sMenu.Get());
		}
	}

	return true;
}

void MenuSystemPlugin::FillMenuEntityKeyValues(CEntityKeyValues *pMenuKV, const Vector &vecOrigin, const QAngle &angRotation, const Vector &vecScales, const Color rgbaColor,  const char *pszFontName, const char *pszBackgroundMaterialName, const char *pszMessageText)
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

void MenuSystemPlugin::FillViewModelEntityKeyValues(CEntityKeyValues *pEntityKV, const Vector &vecOrigin, const QAngle &angRotation)
{
	pEntityKV->SetString("classname", "viewmodel");
	pEntityKV->SetVector("origin", vecOrigin);
	pEntityKV->SetQAngle("angles", angRotation);
}

Vector MenuSystemPlugin::GetEntityPosition(CBaseEntity *pEntity, QAngle *pRotation)
{
	CBodyComponent *pEntityBodyComponent = CBaseEntity_Helper::GetBodyComponentAccessor(pEntity);

	CGameSceneNode *pEntitySceneNode = CBodyComponent_Helper::GetSceneNodeAccessor(pEntityBodyComponent);

	if(pRotation)
	{
		*pRotation = CGameSceneNode_Helper::GetAbsRotationAccessor(pEntitySceneNode);
	}

	return CGameSceneNode_Helper::GetAbsOriginAccessor(pEntitySceneNode);
}

void MenuSystemPlugin::GetMenuEntitiesPosition(const Vector &vecOrigin, const QAngle &angRotation, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	// Correct a rotation to display on the right-side of the screen.
	const float flPitchOffset = 16.f;
	const float flYawOffset = 53.f;

	const QAngle angCorrect = {flPitchOffset, AngleNormalize(angRotation.y + flYawOffset), 90.f};

	vecResult = AddToFrontByRotation(vecOrigin, angCorrect, 16.f);
	vecBackgroundResult = AddToFrontByRotation(vecResult, angCorrect, 0.04f);

	angResult = {0.f, AngleNormalize(angRotation.y - 90.f), 90.f};
}

void MenuSystemPlugin::GetMenuEntitiesPositionByEntity(CBaseEntity *pEntity, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	vecResult = GetEntityPosition(pEntity, &angResult);
	GetMenuEntitiesPosition(vecResult, angResult, vecBackgroundResult, vecResult, angResult);
}

void MenuSystemPlugin::GetMenuEntitiesPositionByViewModel(CBaseViewModel *pViewModel, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	vecResult = GetEntityPosition(pViewModel, &angResult);
	GetMenuEntitiesPosition(vecResult, angResult, vecBackgroundResult, vecResult, angResult);
}

void MenuSystemPlugin::GetMenuEntitiesPositionByCSPlayer(CCSPlayerPawnBase *pCSPlayerPawn, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult)
{
	vecResult = GetEntityPosition(pCSPlayerPawn) + CBaseModelEntity_Helper::GetViewOffsetAccessor(pCSPlayerPawn);
	angResult = CCSPlayerPawnBase_Helper::GetEyeAnglesAccessor(pCSPlayerPawn);
	GetMenuEntitiesPosition(vecResult, angResult, vecBackgroundResult, vecResult, angResult);
}

void MenuSystemPlugin::SpawnEntities(const CUtlVector<CEntityKeyValues *> &vecKeyValues, CUtlVector<CEntityInstance *> *pEntities, IEntityManager::IProviderAgent::IEntityListener *pListener)
{
	if(Logger::Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}

	auto *pEntitySystemAllocator = g_pEntitySystem->GetEntityKeyValuesAllocator();

	const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

	static_assert(INVALID_SPAWN_GROUP == ANY_SPAWN_GROUP);

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

void MenuSystemPlugin::SpawnMenuEntities(const Vector &vecBackgroundOrigin, const Vector &vecOrigin, const QAngle &angRotation, CUtlVector<CEntityInstance *> *pEntities)
{
	auto *pEntitySystemAllocator = g_pEntitySystem->GetEntityKeyValuesAllocator();

	const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

	static_assert(INVALID_SPAWN_GROUP == ANY_SPAWN_GROUP);

	CEntityKeyValues *pMenuKV = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL),
	                 *pMenuKV2 = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL), 
	                 *pMenuKV3 = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL);

	CUtlVector<CEntityKeyValues *> vecKeyValues;

	const Vector vecScales = {0.25f, 0.25f, 0.25f};

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

	FillMenuEntityKeyValues(pMenuKV, vecBackgroundOrigin, angRotation, vecScales, MENU_SYSTEM_BACKGROUND_COLOR, MENU_SYSTEM_DEFAULT_FONT_FAMILY, MENU_SYSTEM_BACKGROUND_MATERIAL_NAME, szMessageTextFull);
	FillMenuEntityKeyValues(pMenuKV2, vecOrigin, angRotation, vecScales, MENU_SYSTEM_ACTIVE_COLOR, MENU_SYSTEM_DEFAULT_FONT_FAMILY, MENU_SYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, szMessageTextActive);
	FillMenuEntityKeyValues(pMenuKV3, vecOrigin, angRotation, vecScales, MENU_SYSTEM_INACTIVE_COLOR, MENU_SYSTEM_DEFAULT_FONT_FAMILY, MENU_SYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME, szMessageTextInactive);

	vecKeyValues.AddToTail(pMenuKV);
	vecKeyValues.AddToTail(pMenuKV2);
	vecKeyValues.AddToTail(pMenuKV3);

	class CMenuEntityListener : public IEntityManager::IProviderAgent::IEntityListener
	{
	public:
		CMenuEntityListener(MenuSystemPlugin *pInitPlugin)
		 :  m_pPlugin(pInitPlugin)
		{
		}

	public:
		void OnEntityCreated(CEntityInstance *pEntity, CEntityKeyValues *pKeyValues)
		{
			if(m_pPlugin->Logger::IsChannelEnabled(LV_DETAILED))
			{
				m_pPlugin->Logger::MessageFormat("Setting up \"%s\" menu entity\n", pEntity->GetClassname());
			}

			m_pPlugin->SettingMenuEntity(reinterpret_cast<CBaseEntity *>(pEntity));
		}

	private:
		MenuSystemPlugin *m_pPlugin;
	} aMenuEntitySetup(this);

	SpawnEntities(vecKeyValues, pEntities, &aMenuEntitySetup);
}

void MenuSystemPlugin::SpawnMenuEntitiesByEntity(CBaseEntity *pEntity, CUtlVector<CEntityInstance *> *pEntities)
{
	Vector vecMenuAbsOriginBackground {},
	       vecMenuAbsOrigin {};

	QAngle angMenuRotation {};

	GetMenuEntitiesPositionByEntity(pEntity, vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation);
	SpawnMenuEntities(vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation, pEntities);
}

// A universal way to create a second view model.
CBaseViewModel *MenuSystemPlugin::SpawnViewModelEntity(const Vector &vecOrigin, const QAngle &angRotation, CBaseEntity *pOwner, const int nSlot)
{
	const SpawnGroupHandle_t hSpawnGroup = m_pMySpawnGroupInstance->GetSpawnGroupHandle();

	static_assert(INVALID_SPAWN_GROUP == ANY_SPAWN_GROUP);

	CEntityKeyValues *pViewModelKV = new CEntityKeyValues(g_pEntitySystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL);

	CUtlVector<CEntityKeyValues *> vecKeyValues;

	CUtlVector<CEntityInstance *> vecEntities;

	class CViewModelEntityListener : public IEntityManager::IProviderAgent::IEntityListener
	{
	public:
		CViewModelEntityListener(MenuSystemPlugin *pInitPlugin, CBaseEntity *pInitOwner, const int nInitSlot)
		 :  m_pPlugin(pInitPlugin),
		    m_pOwner(pInitOwner),
		    m_nSlot(nInitSlot)
		{
		}

	public:
		void OnEntityCreated(CEntityInstance *pEntity, CEntityKeyValues *pKeyValues)
		{
			if(m_pPlugin->Logger::IsChannelEnabled(LV_DETAILED))
			{
				m_pPlugin->Logger::MessageFormat("Setting up \"%s\" view model entity\n", pEntity->GetClassname());
			}

			m_pPlugin->SettingExtraPlayerViewModelEntity(reinterpret_cast<CBaseViewModel *>(pEntity), m_pOwner, m_nSlot);
		}

	private:
		MenuSystemPlugin *m_pPlugin;
		CBaseEntity *m_pOwner;
		const int m_nSlot;
	} aViewModelEntitySetup(this, pOwner, nSlot);

	FillViewModelEntityKeyValues(pViewModelKV, vecOrigin, angRotation);
	vecKeyValues.AddToTail(pViewModelKV);
	SpawnEntities(vecKeyValues, &vecEntities, &aViewModelEntitySetup);

	return reinterpret_cast<CBaseViewModel *>(vecEntities[0]);
}

void MenuSystemPlugin::TeleportMenuEntitiesToCSPlayer(CCSPlayerPawnBase *pCSPlayerPawn, const CUtlVector<CEntityInstance *> &vecEntities)
{
	auto &aBaseEntity = GetGameDataStorage().GetBaseEntity();

	Vector vecMenuAbsOriginBackground {},
	       vecMenuAbsOrigin {};

	QAngle angMenuRotation {};

	GetMenuEntitiesPositionByCSPlayer(pCSPlayerPawn, vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation);

	FOR_EACH_VEC(vecEntities, i)
	{
		auto *pEntity = vecEntities[i];

		aBaseEntity.Teleport(pEntity, i ? vecMenuAbsOrigin : vecMenuAbsOriginBackground, angMenuRotation);
	}
}

bool MenuSystemPlugin::AttachMenuEntitiesToCSPlayer(CCSPlayerPawnBase *pCSPlayerPawn, const CUtlVector<CEntityInstance *> &vecEntities)
{
	auto &aBaseEntity = GetGameDataStorage().GetBaseEntity();

	auto aViewModelServicesAccessor = CCSPlayerPawnBase_Helper::GetViewModelServicesAccessor(pCSPlayerPawn);

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

	GetMenuEntitiesPositionByEntity(pCSPlayerPawn, vecMenuAbsOriginBackground, vecMenuAbsOrigin, angMenuRotation);

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sBuffer;

		sBuffer.Format("Menu entities position:\n");
		aConcat.AppendToBuffer(sBuffer, "Origin", vecMenuAbsOrigin);
		aConcat.AppendToBuffer(sBuffer, "Rotation", angMenuRotation);

		Logger::Detailed(sBuffer);
	}

	if(!pPlayerViewModel)
	{
		auto *pExtraPlayerViewModel = SpawnViewModelEntity(vecMenuAbsOrigin, angMenuRotation, pCSPlayerPawn, s_nExtraViewModelSlot);

		hPlayerViewModel.Set(pExtraPlayerViewModel);
		pPlayerViewModel = pExtraPlayerViewModel;
	}

	aBaseEntity.AcceptInput(pPlayerViewModel, "FollowEntity", pCSPlayerPawn, NULL, &aParentVariant, 0);

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

	aViewModelServicesAccessor.MarkNetworkChanged(); // Update CPlayer_ViewModelServices < CCSPlayer_ViewModelServices state of the view model entities.

	return true;
}

bool MenuSystemPlugin::SettingMenuEntity(CEntityInstance *pEntity)
{
	{
		auto aEFlagsAccessor = CBaseEntity_Helper::GetEFlagsAccessor(reinterpret_cast<CBaseEntity *>(pEntity));

		aEFlagsAccessor = EF_NODRAW;
		aEFlagsAccessor.MarkNetworkChanged();
	}

	return true;
}

bool MenuSystemPlugin::SettingExtraPlayerViewModelEntity(CBaseViewModel *pViewModelEntity, CBaseEntity *pOwner, const int nSlot)
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

bool MenuSystemPlugin::RegisterGameResource(char *error, size_t maxlen)
{
	CGameEntitySystem **pGameEntitySystem = GetGameEntitySystemPointer();

	if(!pGameEntitySystem)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a game entity system", maxlen);
		}
	}

	if(!RegisterGameEntitySystem(*pGameEntitySystem))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a (game) entity system", maxlen);
		}

		return false;
	}

	return true;
}

bool MenuSystemPlugin::UnregisterGameResource(char *error, size_t maxlen)
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

bool MenuSystemPlugin::RegisterGameFactory(char *error, size_t maxlen)
{
	CBaseGameSystemFactory **ppFactory = GetGameDataStorage().GetGameSystem().GetFirstPointer();

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

	m_pFactory = new CGameSystemStaticFactory<MenuSystemPlugin>(GetName(), this);

	return true;
}

bool MenuSystemPlugin::UnregisterGameFactory(char *error, size_t maxlen)
{
	if(m_pFactory)
	{
		m_pFactory->Shutdown();

		// Clean up smart dispatcher listener callbacks.
		{
			const auto *pGameSystem = m_pFactory->GetStaticGameSystem();

			auto **ppDispatcher = GetGameDataStorage().GetGameSystem().GetEventDispatcher();

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

bool MenuSystemPlugin::RegisterSource2Server(char *error, size_t maxlen)
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

bool MenuSystemPlugin::UnregisterSource2Server(char *error, size_t maxlen)
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

bool MenuSystemPlugin::RegisterNetMessages(char *error, size_t maxlen)
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

bool MenuSystemPlugin::UnregisterNetMessages(char *error, size_t maxlen)
{
	m_pSayText2Message = NULL;

	return true;
}

bool MenuSystemPlugin::ParseLanguages(char *error, size_t maxlen)
{
	const char *pszPathID = MENU_SYSTEM_BASE_PATHID, 
	           *pszLanguagesFiles = MENU_SYSTEM_GAME_LANGUAGES_PATH_FILES;

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

bool MenuSystemPlugin::ParseLanguages(KeyValues3 *pRoot, CUtlVector<CUtlString> &vecMessages)
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

bool MenuSystemPlugin::ClearLanguages(char *error, size_t maxlen)
{
	m_vecLanguages.Purge();

	return true;
}

bool MenuSystemPlugin::ParseTranslations(char *error, size_t maxlen)
{
	const char *pszPathID = MENU_SYSTEM_BASE_PATHID, 
	           *pszTranslationsFiles = MENU_SYSTEM_GAME_TRANSLATIONS_PATH_FILES;

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

bool MenuSystemPlugin::ClearTranslations(char *error, size_t maxlen)
{
	Translations::Purge();

	return true;
}

bool MenuSystemPlugin::ParseGameEvents()
{
	const char *pszPathID = MENU_SYSTEM_BASE_PATHID;

	CUtlVector<CUtlString> vecGameEventFiles;

	CUtlVector<CUtlString> vecSubmessages;

	CUtlString sMessage;

	auto aWarnings = Logger::CreateWarningsScope();

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sMessage, NULL, pszPathID}, g_KV3Format_Generic});

	g_pFullFileSystem->FindFileAbsoluteList(vecGameEventFiles, MENU_SYSTEM_GAME_EVENTS_FILES, pszPathID);

	for(const auto &sFile : vecGameEventFiles)
	{
		const char *pszFilename = sFile.Get();

		AnyConfig::Anyone aGameEventConfig;

		aLoadPresets.m_pszFilename = pszFilename;

		if(!aGameEventConfig.Load(aLoadPresets))
		{
			aWarnings.PushFormat("\"%s\": %s", pszFilename, sMessage.Get());

			continue;
		}

		if(!ParseGameEvents(aGameEventConfig.Get(), vecSubmessages))
		{
			aWarnings.PushFormat("\"%s\":", pszFilename);

			for(const auto &sSubmessage : vecSubmessages)
			{
				aWarnings.PushFormat("\t%s", sSubmessage.Get());
			}

			continue;
		}

		// ...
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

bool MenuSystemPlugin::ParseGameEvents(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	int iMemberCount = pData->GetMemberCount();

	if(!iMemberCount)
	{
		vecMessages.AddToTail("No members");

		return false;
	}

	CUtlString sMessage;

	for(KV3MemberId_t n = 0; n < iMemberCount; n++)
	{
		const char *pszEvent = pData->GetMemberName(n);

		if(!pszEvent)
		{
			sMessage.Format("No member name at #%d", n);
			vecMessages.AddToTail(sMessage);

			continue;
		}

		m_vecGameEvents.AddToTail(pszEvent);
	}

	return iMemberCount;
}

bool MenuSystemPlugin::ClearGameEvents()
{
	m_vecGameEvents.Purge();

	return true;
}

bool MenuSystemPlugin::HookGameEvents()
{
	auto aWarnings = Logger::CreateWarningsScope();

	static const char *pszWarningFormat = "Failed to hook \"%s\" event";

	for(const auto &sEvent : m_vecGameEvents)
	{
		const char *pszEvent = sEvent.Get();

		if(g_pGameEventManager->AddListener(static_cast<IGameEventListener2 *>(this), pszEvent, true) == -1)
		{
			aWarnings.PushFormat(pszWarningFormat, pszEvent);

			continue;
		}

#ifdef DEBUG
		Logger::DetailedFormat("Hooked \"%s\" event\n", pszEvent);
#endif
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

bool MenuSystemPlugin::UnhookGameEvents()
{
	g_pGameEventManager->RemoveListener(this);

	return true;
}

void MenuSystemPlugin::OnReloadGameDataCommand(const CCommandContext &context, const CCommand &args)
{
	char error[256];

	if(!LoadProvider(error, sizeof(error)))
	{
		Logger::WarningFormat("%s\n", error);
	}
}

void MenuSystemPlugin::OnMenuSelectCommand(const CCommandContext &context, const CCommand &args)
{
	Logger::MessageFormat("Menu select: slot %d!!!\n", args.ArgC() > 1 ? atoi(args.Arg(1)) : -1);
}

void MenuSystemPlugin::OnDispatchConCommandHook(ConCommandHandle hCommand, const CCommandContext &aContext, const CCommand &aArgs)
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

			bool bIsSilent = *pszArg1 == MenuSystem::ChatCommandSystem::GetSilentTrigger();

			if(bIsSilent || *pszArg1 == MenuSystem::ChatCommandSystem::GetPublicTrigger())
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
						const auto &aConcat = s_aEmbedConcat, 
						           &aConcat2 = s_aEmbed2Concat;

						CBufferStringGrowable<1024> sBuffer;

						sBuffer.Format("Handle a chat command:\n");
						aConcat.AppendToBuffer(sBuffer, "Player slot", aPlayerSlot.Get());
						aConcat.AppendToBuffer(sBuffer, "Is silent", bIsSilent);
						aConcat.AppendToBuffer(sBuffer, "Arguments");

						for(const auto &sArg : vecArgs)
						{
							const char *pszMessageConcat[] = {aConcat2.m_aStartWith, "\"", sArg.Get(), "\"", aConcat2.m_aEnd};

							sBuffer.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
						}

						Logger::Detailed(sBuffer);
					}

					MenuSystem::ChatCommandSystem::Handle(aPlayerSlot, bIsSilent, vecArgs);
				}

				RETURN_META(MRES_SUPERCEDE);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}

void MenuSystemPlugin::OnStartupServerHook(const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char *)
{
	auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

	OnStartupServer(pNetServer, config, pWorldSession);

	RETURN_META(MRES_IGNORED);
}

CServerSideClientBase *MenuSystemPlugin::OnConnectClientHook(const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, 
                                                         const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	auto *pNetServer = META_IFACEPTR(CNetworkGameServerBase);

	auto *pClient = META_RESULT_ORIG_RET(CServerSideClientBase *);

	OnConnectClient(pNetServer, pClient, pszName, pAddr, socket, pSplitPlayer, pszChallenge, pAuthTicket, nAuthTicketLength, bIsLowViolence);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bool MenuSystemPlugin::OnProcessRespondCvarValueHook(const CCLCMsg_RespondCvarValue_t &aMessage)
{
	auto *pClient = META_IFACEPTR(CServerSideClientBase);

	OnProcessRespondCvarValue(pClient, aMessage);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void MenuSystemPlugin::OnDisconectClientHook(ENetworkDisconnectionReason eReason)
{
	auto *pClient = META_IFACEPTR(CServerSideClientBase);

	OnDisconectClient(pClient, eReason);

	RETURN_META(MRES_IGNORED);
}

void MenuSystemPlugin::SendCvarValueQuery(IRecipientFilter *pFilter, const char *pszName, int iCookie)
{
	auto *pGetCvarValueMessage = m_pGetCvarValueMessage;

	auto *pMessage = pGetCvarValueMessage->AllocateMessage()->ToPB<CSVCMsg_GetCvarValue>();

	pMessage->set_cvar_name(pszName);
	pMessage->set_cookie(iCookie);

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pGetCvarValueMessage, pMessage, 0);

	delete pMessage;
}

void MenuSystemPlugin::SendChatMessage(IRecipientFilter *pFilter, int iEntityIndex, bool bIsChat, const char *pszChatMessageFormat, const char *pszParam1, const char *pszParam2, const char *pszParam3, const char *pszParam4)
{
	auto *pSayText2Message = m_pSayText2Message;

	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sBuffer;

		sBuffer.Format("Send chat message (%s):\n", pSayText2Message->GetUnscopedName());
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

void MenuSystemPlugin::SendTextMessage(IRecipientFilter *pFilter, int iDestination, size_t nParamCount, const char *pszParam, ...)
{
	auto *pTextMsg = m_pTextMsgMessage;

	if(Logger::IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sBuffer;

		sBuffer.Format("Send message (%s):\n", pTextMsg->GetUnscopedName());
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

// void MenuSystemPlugin::SendVGUIMenuMessage(IRecipientFilter *pFilter, const char *pszName, const bool *pIsShow, KeyValues3 *pKeys)
// {
// 	auto *pVGUIMenuMsg = m_pVGUIMenuMessage;

// 	if(Logger::IsChannelEnabled(LV_DETAILED))
// 	{
// 		const auto &aConcat = s_aEmbedConcat, 
// 		           &aConcat2 = s_aEmbed2Concat;

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

void MenuSystemPlugin::OnStartupServer(CNetworkGameServerBase *pNetServer, const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession)
{
	SH_ADD_HOOK_MEMFUNC(CNetworkGameServerBase, ConnectClient, pNetServer, this, &MenuSystemPlugin::OnConnectClientHook, true);

	// Initialize & hook game evetns.
	// Initialize network messages.
	{
		char sMessage[256];

		if(RegisterSource2Server(sMessage, sizeof(sMessage)))
		{
			HookGameEvents();
		}
		else
		{
			Logger::WarningFormat("%s\n", sMessage);
		}

		if(!RegisterNetMessages(sMessage, sizeof(sMessage)))
		{
			Logger::WarningFormat("%s\n", sMessage);
		}
	}
}

void MenuSystemPlugin::OnConnectClient(CNetworkGameServerBase *pNetServer, CServerSideClientBase *pClient, const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	if(pClient)
	{
		SH_ADD_HOOK_MEMFUNC(CServerSideClientBase, PerformDisconnection, pClient, this, &MenuSystemPlugin::OnDisconectClientHook, false);

		if(pClient->IsFakeClient())
		{
			return;
		}

		SH_ADD_HOOK_MEMFUNC(CServerSideClientBase, ProcessRespondCvarValue, pClient, this, &MenuSystemPlugin::OnProcessRespondCvarValueHook, false);
	}
	else
	{
		AssertMsg(0, "Failed to get a server side client pointer\n");

		return;
	}

	auto *pPlayer = reinterpret_cast<CServerSideClient *>(pClient);

	auto aSlot = pClient->GetPlayerSlot();

	auto &aPlayer = GetPlayerData(aSlot);

	// Get "cl_language" cvar value from a client.
	{

		CSingleRecipientFilter aFilter(aSlot);

		const char *pszCvarName = MENU_SYSTEM_CLIENT_CVAR_NAME_LANGUAGE;

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

bool MenuSystemPlugin::OnProcessRespondCvarValue(CServerSideClientBase *pClient, const CCLCMsg_RespondCvarValue_t &aMessage)
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

void MenuSystemPlugin::OnDisconectClient(CServerSideClientBase *pClient, ENetworkDisconnectionReason eReason)
{
	SH_REMOVE_HOOK_MEMFUNC(CServerSideClientBase, PerformDisconnection, pClient, this, &MenuSystemPlugin::OnDisconectClientHook, false);

	if(pClient->IsFakeClient())
	{
		return;
	}

	SH_REMOVE_HOOK_MEMFUNC(CServerSideClientBase, ProcessRespondCvarValue, pClient, this, &MenuSystemPlugin::OnProcessRespondCvarValueHook, false);

	auto *pPlayer = reinterpret_cast<CServerSideClient *>(pClient);

	auto aSlot = pClient->GetPlayerSlot();

	auto &aPlayer = GetPlayerData(aSlot);

	aPlayer.OnDisconnected(pPlayer, eReason);
}

CUtlSymbolLarge MenuSystemPlugin::GetConVarSymbol(const char *pszName)
{
	return m_tableConVars.AddString(pszName);
}

CUtlSymbolLarge MenuSystemPlugin::FindConVarSymbol(const char *pszName) const
{
	return m_tableConVars.Find(pszName);
}

CUtlSymbolLarge MenuSystemPlugin::GetLanguageSymbol(const char *pszName)
{
	return m_tableLanguages.AddString(pszName);
}

CUtlSymbolLarge MenuSystemPlugin::FindLanguageSymbol(const char *pszName) const
{
	return m_tableLanguages.Find(pszName);
}
