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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_CHATSYSTEM_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_CHATSYSTEM_HPP_

#	pragma once

#	include "chatcommandsystem.hpp"

#	include <playerslot.h>
#	include <tier0/bufferstring.h>
#	include <tier0/utlstring.h>
#	include <tier1/utlvector.h>

#	include <logger.hpp>

#	define MENU_CHATSYSTEM_BASE_DIR "configs" CORRECT_PATH_SEPARATOR_S "menu" CORRECT_PATH_SEPARATOR_S "chatsystem"
#	define MENU_CHATSYSTEM_ALIASES_FILENAME MENU_CHATSYSTEM_BASE_DIR CORRECT_PATH_SEPARATOR_S "aliases.*"

class KeyValues3;

namespace Menu
{
	using ChatSystemBase = ChatCommandSystem;

	class ChatSystem : public ChatSystemBase
	{
	public:
		using Base = ChatSystemBase;

		using AliasKey_t = CUtlSymbolLarge;
		using AliasValue_t = CUtlSymbolLarge;

		ChatSystem();

	public:
		bool Load(const char *pszBaseGameDir, const char *pszPathID, CUtlVector<CUtlString> &vecMessages);
		void Clear();

	protected:
		bool LoadAliases(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);

	public:
		int ReplaceString(CBufferString &sBuffer);

	protected:
		CUtlSymbolLarge GetAliasSymbol(const char *pszName);
		CUtlSymbolLarge FindAliasSymbol(const char *pszName) const;

	private:
		CUtlSymbolTableLarge m_tableAliases;
		CUtlMap<AliasKey_t, AliasValue_t> m_mapAliases;

	protected:
		CUtlSymbolLarge GetAliasValueSymbol(const char *pszValue);
		CUtlSymbolLarge FindAliasValueSymbol(const char *pszValue) const;

	private:
		CUtlSymbolTableLarge m_tableAliasValues;
	}; // Menu::ChatSystem
}; // Menu

#endif // _INCLUDE_METAMOD_SOURCE_MENU_CHATSYSTEM_HPP_
