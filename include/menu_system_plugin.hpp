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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PLUGIN_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PLUGIN_HPP_

#	pragma once

#	include "imenu.hpp"
#	include "ientitymgr.hpp"
#	include "menu_system/chat_command_system.hpp"
#	include "menu_system/provider.hpp"
#	include "menu_system/schema.hpp"
#	include "menu_system/schema/base_entity.hpp"
#	include "menu_system/schema/base_model_entity.hpp"
#	include "menu_system/schema/base_player_controller.hpp"
#	include "menu_system/schema/base_player_pawn.hpp"
#	include "menu_system/schema/base_view_model.hpp"
#	include "menu_system/schema/body_component.hpp"
#	include "menu_system/schema/cs_player_pawn_base.hpp"
#	include "menu_system/schema/cs_player_view_model_services.hpp"
#	include "menu_system/schema/game_scene_node.hpp"
#	include "concat.hpp"

#	include <logger.hpp>
#	include <translations.hpp>

#	include <ISmmPlugin.h>

#	include <bitvec.h>
#	include <const.h>
#	include <igameevents.h>
#	include <igamesystem.h>
#	include <igamesystemfactory.h>
#	include <iloopmode.h>
#	include <iserver.h>
#	include <netmessages.h>
#	include <playerslot.h>
#	include <tier0/bufferstring.h>
#	include <tier0/strtools.h>
#	include <tier1/convar.h>
#	include <tier1/utlvector.h>
#	include <tier1/utlmap.h>
#	include <tier1/keyvalues3.h>
#	include <entity2/entitykeyvalues.h>
#	include <network_connection.pb.h>

#	define MENU_SYSTEM_LOGGINING_COLOR {127, 255, 0, 191} // Green (Chartreuse)
#	define MENU_SYSTEM_MAX_MESSAGE_TEXT_LENGTH 512
#	define MENU_SYSTEM_MAX_FONT_NAME_LENGTH 64
#	define MENU_SYSTEM_MAX_BACKGROUND_MATERIAL_NAME_LENGTH 64
#	define MENU_SYSTEM_BACKGROUND_COLOR {100, 73, 28, 255}
#	define MENU_SYSTEM_ACTIVE_COLOR {195, 141, 52, 255}
#	define MENU_SYSTEM_INACTIVE_COLOR {255, 255, 255, 255}
#	define MENU_SYSTEM_DEFAULT_FONT_FAMILY "Arial"
#	define MENU_SYSTEM_BACKGROUND_MATERIAL_NAME "materials/dev/annotation_worldtext_background.vmat"
#	define MENU_SYSTEM_EMPTY_BACKGROUND_MATERIAL_NAME "materials/editor/icon_empty.vmat"
#	define MENU_SYSTEM_TEST_MESSAGE_TEXT

#	define MENU_SYSTEM_BASE_DIR "addons" CORRECT_PATH_SEPARATOR_S META_PLUGIN_PREFIX
#	define MENU_SYSTEM_GAME_EVENTS_FILES "resource" CORRECT_PATH_SEPARATOR_S "*.gameevents"
#	define MENU_SYSTEM_GAME_TRANSLATIONS_FILES "translations" CORRECT_PATH_SEPARATOR_S "*.phrases.*"
#	define MENU_SYSTEM_GAME_TRANSLATIONS_PATH_FILES MENU_SYSTEM_BASE_DIR CORRECT_PATH_SEPARATOR_S MENU_SYSTEM_GAME_TRANSLATIONS_FILES
#	define MENU_SYSTEM_GAME_LANGUAGES_FILES "configs" CORRECT_PATH_SEPARATOR_S "languages.*"
#	define MENU_SYSTEM_GAME_LANGUAGES_PATH_FILES MENU_SYSTEM_BASE_DIR CORRECT_PATH_SEPARATOR_S MENU_SYSTEM_GAME_LANGUAGES_FILES
#	define MENU_SYSTEM_BASE_PATHID "GAME"

#	define MENU_SYSTEM_EXAMPLE_CHAT_COMMAND "example"

#	define MENU_SYSTEM_CLIENT_CVAR_NAME_LANGUAGE "cl_language"

class CBasePlayerController;
class INetworkMessageInternal;

class MenuSystemPlugin final : public ISmmPlugin, public IMetamodListener, public IMenuSystem, public CBaseGameSystem, public IGameEventListener2, public IEntityManager::IProviderAgent::ISpawnGroupNotifications, // Interfaces.
                               public MenuSystem::ChatCommandSystem, public MenuSystem::Provider, virtual public MenuSystem::Schema::CSystem, virtual public Logger, public Translations, // Conponents.
                               virtual public MenuSystem::Schema::CBaseEntity_Helper, virtual public MenuSystem::Schema::CBaseModelEntity_Helper, virtual public MenuSystem::Schema::CBasePlayerController_Helper, virtual public MenuSystem::Schema::CBaseViewModel_Helper, virtual public MenuSystem::Schema::CBodyComponent_Helper, virtual public MenuSystem::Schema::CCSPlayerPawnBase_Helper, virtual public MenuSystem::Schema::CGameSceneNode_Helper, virtual public MenuSystem::Schema::CCSPlayer_ViewModelServices_Helper // Schema helpers.
{
public:
	MenuSystemPlugin();

public: // ISmmPlugin
	bool Load(PluginId id, ISmmAPI *ismm, char *error = nullptr, size_t maxlen = 0, bool late = true) override;
	bool Unload(char *error, size_t maxlen) override;
	bool Pause(char *error, size_t maxlen) override;
	bool Unpause(char *error, size_t maxlen) override;
	void AllPluginsLoaded() override;

	const char *GetAuthor() override;
	const char *GetName() override;
	const char *GetDescription() override;
	const char *GetURL() override;
	const char *GetLicense() override;
	const char *GetVersion() override;
	const char *GetDate() override;
	const char *GetLogTag() override;

public: // IMetamodListener
	void *OnMetamodQuery(const char *iface, int *ret) override;

public: // IMenuSystem
	CGameEntitySystem **GetGameEntitySystemPointer() const override;
	CBaseGameSystemFactory **GetFirstGameSystemPointer() const override;
	CGameSystemEventDispatcher **GetGameSystemEventDispatcherPointer() const override;
	IGameEventManager2 **GetGameEventManagerPointer() const override;

	class CLanguage : public IMenuSystem::ILanguage
	{
		friend class MenuSystemPlugin;

	public:
		CLanguage(const CUtlSymbolLarge &sInitName = NULL, const char *pszInitCountryCode = "en");

	public:
		const char *GetName() const override;
		const char *GetCountryCode() const override;

	protected:
		void SetName(const CUtlSymbolLarge &sInitName);
		void SetCountryCode(const char *psz);

	private:
		CUtlSymbolLarge m_sName;
		CUtlString m_sCountryCode;
	}; // MenuSystemPlugin::CLanguage

	class CPlayer : public IPlayer
	{
		friend class MenuSystemPlugin;

	public:
		CPlayer();

	public: // ISample::IPlayerLanguageCallbacks
		bool AddLanguageListener(IPlayerLanguageListener *pListener) override;
		bool RemoveLanguageListener(IPlayerLanguageListener *pListener) override;

	public: // ISample::IPlayerLanguage
		const ILanguage *GetLanguage() const override;
		void SetLanguage(const ILanguage *pData) override;

	public: // ISample::IPlayerBase
		bool IsConnected() override;
		CServerSideClient *GetServerSideClient() override;

	public: // IMenuSystem::IPlayer
		CUtlVector<CEntityInstance *> &GetMenuEntities() override;

	public:
		virtual void OnConnected(CServerSideClient *pClient);
		virtual void OnDisconnected(CServerSideClient *pClient, ENetworkDisconnectionReason eReason);

	public:
		virtual void OnLanguageChanged(CPlayerSlot aSlot, CLanguage *pData);

	public:
		struct TranslatedPhrase
		{
			const Translations::CPhrase::CFormat *m_pFormat;
			const Translations::CPhrase::CContent *m_pContent;
		};

		void TranslatePhrases(const Translations *pTranslations, const CLanguage &aServerLanguage, CUtlVector<CUtlString> &vecMessages);
		const TranslatedPhrase &GetYourArgumentPhrase() const;

	private:
		CServerSideClient *m_pServerSideClient;

	private: // Menus data.
		CUtlVector<CEntityInstance *> m_vecMenuEntities;

	private:
		const ILanguage *m_pLanguage;
		CUtlVector<IPlayerLanguageListener *> m_vecLanguageCallbacks;

	private:
		TranslatedPhrase m_aYourArgumentPhrase;
	}; // MenuSystemPlugin::CPlayerBase

	const ILanguage *GetServerLanguage() const override;
	const ILanguage *GetLanguageByName(const char *psz) const override;
	IPlayerBase *GetPlayerBase(const CPlayerSlot &aSlot) override;
	IPlayer *GetPlayer(const CPlayerSlot &aSlot) override;
	CPlayer &GetPlayerData(const CPlayerSlot &aSlot);

public: // CBaseGameSystem
	bool Init() override;
	void PostInit() override;
	void Shutdown() override;

	GS_EVENT(GameActivate);
	GS_EVENT(GameDeactivate);
	GS_EVENT(ServerPostEntityThink);

public: // IGameEventListener2
	void FireGameEvent(IGameEvent *event) override;
	bool OnPlayerTeam(IGameEvent *event);

public: // IEntityManager::IProviderAgent::ISpawnGroupNotifications
	void OnSpawnGroupAllocated(SpawnGroupHandle_t hSpawnGroup, ISpawnGroup *pSpawnGroup) override;
	void OnSpawnGroupInit(SpawnGroupHandle_t hSpawnGroup, IEntityResourceManifest *pManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry) override;
	void OnSpawnGroupCreateLoading(SpawnGroupHandle_t hSpawnGroup, CMapSpawnGroup *pMapSpawnGroup, bool bSynchronouslySpawnEntities, bool bConfirmResourcesLoaded, CUtlVector<const CEntityKeyValues *> &vecKeyValues) override;
	void OnSpawnGroupDestroyed(SpawnGroupHandle_t hSpawnGroup) override;

public: // Utils.
	bool InitProvider(char *error = nullptr, size_t maxlen = 0);
	bool LoadProvider(char *error = nullptr, size_t maxlen = 0);
	bool UnloadProvider(char *error = nullptr, size_t maxlen = 0);

public: // Schema.
	bool InitSchema(char *error = nullptr, size_t maxlen = 0);
	bool UnloadSchema(char *error = nullptr, size_t maxlen = 0);

public: // Entity Manager.
	bool InitEntityManager(char *error = nullptr, size_t maxlen = 0);
	void DumpEntityManager(const ConcatLineString &aConcat, CBufferString &sOutput);
	bool UnloadEntityManager(char *error = nullptr, size_t maxlen = 0);

	bool LoadMenuSpawnGroups(const Vector &aWorldOrigin = {0.0f, 0.0f, 0.0f});

	// Entity keyvalues.
	void FillMenuEntityKeyValues(CEntityKeyValues *pMenuKV, const Vector &vecOrigin, const QAngle &angRotation, const Vector &vecScales, const Color rgbaColor, const char *pszFontName, const char *pszBackgroundMaterialName, const char *pszMessageText);
	void FillViewModelEntityKeyValues(CEntityKeyValues *pEntityKV, const Vector &vecOrigin, const QAngle &angRotation);

	// Get & calculate positions.
	Vector GetEntityPosition(CBaseEntity *pTarget, QAngle *pRotation = nullptr);
	void CalculateMenuEntitiesPosition(const Vector &vecOrigin, const QAngle &angRotation, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult);
	void CalculateMenuEntitiesPositionByEntity(CBaseEntity *pTarget, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult);
	void CalculateMenuEntitiesPositionByViewModel(CBaseViewModel *pTarget, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult);
	void CalculateMenuEntitiesPositionByCSPlayer(CCSPlayerPawnBase *pTarget, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult);

	// Spawn entities.
	void SpawnEntities(const CUtlVector<CEntityKeyValues *> &vecKeyValues, CUtlVector<CEntityInstance *> *pEntities = nullptr, IEntityManager::IProviderAgent::IEntityListener *pListener = nullptr);
	void SpawnMenuEntities(const Vector &vecBackgroundOrigin, const Vector &vecOrigin, const QAngle &angRotation, CUtlVector<CEntityInstance *> *pEntities);
	void SpawnMenuEntitiesByEntity(CBaseEntity *pTarget, CUtlVector<CEntityInstance *> *pEntities);
	CBaseViewModel *SpawnViewModelEntity(const Vector &vecOrigin, const QAngle &angRotation, CBaseEntity *pOwner, const int nSlot);

	// Menus movement.
	void TeleportMenuEntitiesToCSPlayer(CCSPlayerPawnBase *pTarget, const CUtlVector<CEntityInstance *> &vecEntities);
	void AttachMenuEntitiesToEntity(CBaseEntity *pTarget, const CUtlVector<CEntityInstance *> &vecEntities);
	bool AttachMenuEntitiesToCSPlayer(CCSPlayerPawnBase *pTarget, const CUtlVector<CEntityInstance *> &vecEntities);

	// Setting up the entities.
	bool SettingMenuEntity(CEntityInstance *pEntity);
	bool SettingExtraPlayerViewModelEntity(CBaseViewModel *pEntity, CBaseEntity *pOwner, const int nSlot);

public: // Game Resource.
	bool RegisterGameResource(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterGameResource(char *error = nullptr, size_t maxlen = 0);

public: // Game Factory.
	bool RegisterGameFactory(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterGameFactory(char *error = nullptr, size_t maxlen = 0);

public: // Source 2 Server.
	bool RegisterSource2Server(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterSource2Server(char *error = nullptr, size_t maxlen = 0);

public: // Network Messages.
	bool RegisterNetMessages(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterNetMessages(char *error = nullptr, size_t maxlen = 0);

public: // Languages.
	bool ParseLanguages(char *error = nullptr, size_t maxlen = 0);
	bool ParseLanguages(KeyValues3 *pRoot, CUtlVector<CUtlString> &vecMessages);
	bool ClearLanguages(char *error = nullptr, size_t maxlen = 0);

public: // Translations.
	bool ParseTranslations(char *error = nullptr, size_t maxlen = 0);
	bool ClearTranslations(char *error = nullptr, size_t maxlen = 0);

public: // Event actions.
	bool ParseGameEvents();
	bool ParseGameEvents(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages); // Parse the structure of events.
	bool ClearGameEvents();

	bool HookGameEvents();
	bool UnhookGameEvents();

private: // Commands.
	CON_COMMAND_MEMBER_F(MenuSystemPlugin, "mm_" META_PLUGIN_PREFIX "_reload_gamedata", OnReloadGameDataCommand, "Reload gamedata configs", FCVAR_LINKED_CONCOMMAND);
	CON_COMMAND_MEMBER_F(MenuSystemPlugin, "menuselect", OnMenuSelectCommand, "", FCVAR_LINKED_CONCOMMAND | FCVAR_CLIENT_CAN_EXECUTE);

private: // ConVars. See the constructor
	ConVar<bool> m_aEnableGameEventsDetaillsConVar;

public: // SourceHooks.
	void OnStartupServerHook(const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char *);
	void OnDispatchConCommandHook(ConCommandHandle hCommand, const CCommandContext &aContext, const CCommand &aArgs);
	CServerSideClientBase *OnConnectClientHook(const char *pszName, ns_address *pAddr, void *pNetInfo, C2S_CONNECT_Message *pConnectMsg, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
	bool OnProcessRespondCvarValueHook(const CCLCMsg_RespondCvarValue_t &aMessage);
	void OnDisconectClientHook(ENetworkDisconnectionReason eReason);

public: // Utils.
	void SendCvarValueQuery(IRecipientFilter *pFilter, const char *pszName, int iCookie);
	void SendChatMessage(IRecipientFilter *pFilter, int iEntityIndex, bool bIsChat, const char *pszChatMessageFormat, const char *pszParam1 = "", const char *pszParam2 = "", const char *pszParam3 = "", const char *pszParam4 = "");
	void SendTextMessage(IRecipientFilter *pFilter, int iDestination, size_t nParamCount, const char *pszParam, ...);
	// void SendVGUIMenuMessage(IRecipientFilter *pFilter, const char *pszName = nullptr, const bool *pIsShow = nullptr, KeyValues3 *pKeys = nullptr);

protected: // Handlers.
	void OnStartupServer(CNetworkGameServerBase *pNetServer, const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession);
	void OnConnectClient(CNetworkGameServerBase *pNetServer, CServerSideClientBase *pClient, const char *pszName, ns_address *pAddr, void *pNetInfo, C2S_CONNECT_Message *pConnectMsg, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
	bool OnProcessRespondCvarValue(CServerSideClientBase *pClient, const CCLCMsg_RespondCvarValue_t &aMessage);
	void OnDisconectClient(CServerSideClientBase *pClient, ENetworkDisconnectionReason eReason);

protected: // ConVar symbols.
	CUtlSymbolLarge GetConVarSymbol(const char *pszName);
	CUtlSymbolLarge FindConVarSymbol(const char *pszName) const;

private: // ConVar (hash)map.
	CUtlSymbolTableLarge_CI m_tableConVars;
	CUtlMap<CUtlSymbolLarge, int> m_mapConVarCookies;

protected: // Language symbols.
	CUtlSymbolLarge GetLanguageSymbol(const char *pszName);
	CUtlSymbolLarge FindLanguageSymbol(const char *pszName) const;

private: // Language (hash)map.
	CUtlSymbolTableLarge_CI m_tableLanguages;
	CUtlMap<CUtlSymbolLarge, CLanguage> m_mapLanguages;

private: // Fields.
	CGameSystemStaticFactory<MenuSystemPlugin> *m_pFactory = NULL;

	CKeyValues3Context m_aEntityAllocator;

	// Provide to Entity Manager plugin.
	IEntityManager *m_pEntityManager = nullptr;
	IEntityManager::IProviderAgent *m_pEntityManagerProviderAgent = nullptr;
	IEntityManager::CSpawnGroupProvider *m_pEntityManagerSpawnGroupProvider = nullptr;

	// Run-time things.
	IEntityManager::IProviderAgent::ISpawnGroupInstance *m_pMySpawnGroupInstance;

	INetworkMessageInternal *m_pGetCvarValueMessage = NULL;
	INetworkMessageInternal *m_pSayText2Message = NULL;
	INetworkMessageInternal *m_pTextMsgMessage = NULL;
	// INetworkMessageInternal *m_pVGUIMenuMessage = NULL;

	CUtlVector<CUtlString> m_vecGameEvents;

	CLanguage m_aServerLanguage;
	CUtlVector<CLanguage> m_vecLanguages;

	CPlayer m_aPlayers[ABSOLUTE_PLAYER_LIMIT];
}; // MenuSystemPlugin

extern MenuSystemPlugin *g_pMenuSystemPlugin;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PLUGIN_HPP_
