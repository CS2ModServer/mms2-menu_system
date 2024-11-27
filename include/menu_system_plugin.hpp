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

#	include <imenu.hpp>
#	include <ientitymgr.hpp>
#	include <menu_system/chat_command_system.hpp>
#	include <menu_system/provider.hpp>
#	include <menu_system/schema.hpp>
#	include <menu_system/schema/base_entity.hpp>
#	include <menu_system/schema/base_model_entity.hpp>
#	include <menu_system/schema/base_player_controller.hpp>
#	include <menu_system/schema/body_component.hpp>
#	include <menu_system/schema/cs_player_pawn_base.hpp>
#	include <menu_system/schema/game_scene_node.hpp>
#	include <concat.hpp>

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

#	define MENU_SYSTEM_LOGGINING_COLOR {127, 255, 0, 191} // Green (Chartreuse)

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
                               public MenuSystem::ChatCommandSystem, public MenuSystem::Provider, public MenuSystem::CSchemaSystem_Helper, virtual public Logger, public Translations, // Conponents.
                               public MenuSystem::Schema::CBaseEntity_Helper, public MenuSystem::Schema::CBaseModelEntity_Helper, public MenuSystem::Schema::CBasePlayerController_Helper, public MenuSystem::Schema::CBodyComponent_Helper, public MenuSystem::Schema::CCSPlayerPawnBase_Helper, public MenuSystem::Schema::CGameSceneNode_Helper // Schema helpers.
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
	}; // CLanguage

	class CPlayer : public IPlayer
	{
		friend class MenuSystemPlugin;

	public:
		CPlayer();

	public:
		void Init();
		void Destroy();

	public: // IPlayerLanguageCallbacks
		bool AddLanguageListener(const LanguageHandleCallback_t &fnCallback) override;
		bool RemoveLanguageListener(const LanguageHandleCallback_t &fnCallback) override;

	public: // IPlayerLanguage
		const ILanguage *GetLanguage() const override;
		void SetLanguage(const ILanguage *pData) override;

	public:
		virtual void OnLanguageReceived(CPlayerSlot aSlot, CLanguage *pData);

	public:
		struct TranslatedPhrase
		{
			const Translations::CPhrase::CFormat *m_pFormat;
			const Translations::CPhrase::CContent *m_pContent;
		};

		void TranslatePhrases(const Translations *pTranslations, const CLanguage &aServerLanguage, CUtlVector<CUtlString> &vecMessages);
		const TranslatedPhrase &GetYourArgumentPhrase() const;

	private:
		const ILanguage *m_pLanguage;
		CUtlVector<const LanguageHandleCallback_t *> m_vecLanguageCallbacks;

	private:
		TranslatedPhrase m_aYourArgumentPhrase;
	}; // CPlayer

	const IMenuSystem::ILanguage *GetServerLanguage() const override;
	const IMenuSystem::ILanguage *GetLanguageByName(const char *psz) const override;
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
	void FillMenuEntityKeyValues(CEntityKeyValues *pMenuKV, const Vector &vecOrigin, const QAngle &angRotation);
	void FillMenuEntityKeyValues2(CEntityKeyValues *pMenuKV, const Vector &vecOrigin, const QAngle &angRotation);
	void FillMenuEntityKeyValues3(CEntityKeyValues *pMenuKV, const Vector &vecOrigin, const QAngle &angRotation);

	void GetMenuEntitiesPosition(const Vector &vecOrigin, const QAngle &angRotation, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult);
	void GetMenuEntitiesPositionByPlayer(CBasePlayerPawn *pPlayerPawn, Vector &vecBackgroundResult, Vector &vecResult, QAngle &angResult);

	void SpawnMenuEntities(const Vector &vecBackgroundOrigin, const Vector &vecOrigin, const QAngle &angRotation, CUtlVector<CEntityInstance *> *pEntities);
	void SpawnMenuEntitiesByPlayer(CBasePlayerPawn *pPlayerPawn, CUtlVector<CEntityInstance *> *pEntities);

	void TeleportMenuEntitiesToPlayer(CBasePlayerPawn *pPlayerPawn, const CUtlVector<CEntityInstance *> &vecEntities);
	void AttachMenuEntitiesToPlayer(CBasePlayerPawn *pPlayerPawn, const CUtlVector<CEntityInstance *> &vecEntities);

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

private: // ConVars. See the constructor
	ConVar<bool> m_aEnableFrameDetailsConVar;
	ConVar<bool> m_aEnableGameEventsDetaillsConVar;

public: // SourceHooks.
	void OnStartupServerHook(const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char *);
	void OnDispatchConCommandHook(ConCommandHandle hCommand, const CCommandContext &aContext, const CCommand &aArgs);
	CServerSideClientBase *OnConnectClientHook(const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
	bool OnProcessRespondCvarValueHook(const CCLCMsg_RespondCvarValue_t &aMessage);
	void OnDisconectClientHook(ENetworkDisconnectionReason eReason);

public: // Utils.
	void SendCvarValueQuery(IRecipientFilter *pFilter, const char *pszName, int iCookie);
	void SendChatMessage(IRecipientFilter *pFilter, int iEntityIndex, bool bIsChat, const char *pszChatMessageFormat, const char *pszParam1 = "", const char *pszParam2 = "", const char *pszParam3 = "", const char *pszParam4 = "");
	void SendTextMessage(IRecipientFilter *pFilter, int iDestination, size_t nParamCount, const char *pszParam, ...);

protected: // Handlers.
	void OnStartupServer(CNetworkGameServerBase *pNetServer, const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession);
	void OnConnectClient(CNetworkGameServerBase *pNetServer, CServerSideClientBase *pClient, const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
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
	IGameSystemFactory *m_pFactory = NULL;

	CKeyValues3Context m_aEntityAllocator;

	// Provide to Entity Manager plugin.
	IEntityManager *m_pEntityManager = nullptr;
	IEntityManager::IProviderAgent *m_pEntityManagerProviderAgent = nullptr;
	IEntityManager::CSpawnGroupProvider *m_pEntityManagerSpawnGroupProvider = nullptr;

	// Run-time things.
	IEntityManager::IProviderAgent::ISpawnGroupInstance *m_pMySpawnGroupInstance;

	CUtlVector<CEntityInstance *> m_vecMyEntities;
	CUtlMap<int /* Player slot index. */, CUtlVector<CEntityInstance *>> m_mapPlayerEntities;

	INetworkMessageInternal *m_pGetCvarValueMessage = NULL;
	INetworkMessageInternal *m_pSayText2Message = NULL;
	INetworkMessageInternal *m_pTextMsgMessage = NULL;

	CUtlVector<CUtlString> m_vecGameEvents;

	CLanguage m_aServerLanguage;
	CUtlVector<CLanguage> m_vecLanguages;

	CPlayer m_aPlayers[ABSOLUTE_PLAYER_LIMIT];
}; // MenuSystemPlugin

extern MenuSystemPlugin *g_pMenuSystemPlugin;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PLUGIN_HPP_
