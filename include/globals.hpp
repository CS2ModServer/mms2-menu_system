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

#ifndef _INCLUDE_METAMOD_SOURCE_GLOBALS_HPP_
#	define _INCLUDE_METAMOD_SOURCE_GLOBALS_HPP_

#	pragma once

#	include <stddef.h>

#	include <tier0/bufferstring.h>

class CConcatLineString;

class IVEngineServer2;
class ICvar;
class IFileSystem;
class ISource2Server;

namespace SourceMM
{
	class ISmmAPI;
	class ISmmPlugin;
}; // SourceMM

class CGlobalVars;
class IGameEventSystem;
class CEntitySystem;
class CGameEntitySystem;
class CBaseGameSystemFactory;
class CGameEventManager;
class IGameEventManager2;
class CNetworkGameServerBase;

#	include <interfaces/interfaces.h>
#	include <igamesystemfactory.h>

#	define GLOBALS_APPEND_VARIABLE(concatVar, var) aConcat.AppendPointerToBuffer(sOutput, #var, var);

extern IGameEventSystem *g_pGameEventSystem;
extern CEntitySystem *g_pEntitySystem;
extern CGameEntitySystem *g_pGameEntitySystem;
extern IGameEventManager2 *g_pGameEventManager;
extern CNetworkGameServerBase *g_pNetworkGameServer;

extern bool InitGlobals(SourceMM::ISmmAPI *ismm, char *error = nullptr, size_t maxlen = 0);

extern bool RegisterGameEntitySystem(CGameEntitySystem *pGameEntitySystem);
extern bool UnregisterGameEntitySystem();

extern bool RegisterFirstGameSystem(CBaseGameSystemFactory **ppFirstGameSystem);
extern bool UnregisterFirstGameSystem();

extern bool RegisterGameEventManager(IGameEventManager2 *pGameEventManager);
extern bool UnregisterGameEventManager();

extern void DumpGlobals(const CConcatLineString &aConcat, CBufferString &sOutput);
extern void DumpRegisterGlobals(const CConcatLineString &aConcat, CBufferString &sOutput);
extern bool DestoryGlobals(char *error = nullptr, size_t maxlen = 0);

extern CGlobalVars *GetGameGlobals();
// CGameEntitySystem *GameEntitySystem(); // Declared in <entity2/entitysystem.h>

#endif //_INCLUDE_METAMOD_SOURCE_GLOBALS_HPP_
