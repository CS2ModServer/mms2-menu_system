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
 * GNU General Public License for more details.S

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_PROFILE_SYSTEM_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_PROFILE_SYSTEM_HPP_

#	pragma once

#	include <imenuprofilesystem.hpp>
#	include "systembase.hpp"

#	include <tier0/platform.h>

#	include <playerslot.h>
#	include <tier0/bufferstring.h>
#	include <tier0/utlstring.h>
#	include <tier1/utlmap.h>
#	include <tier1/utlsymbollarge.h>
#	include <tier1/utlvector.h>
#	include <tier1/keyvalues3.h>

#	include <logger.hpp>

#	define MENU_SYSTEM_PROFILES_FILENAME MENU_SYSTEM_BASE_DIR CORRECT_PATH_SEPARATOR_S "profiles.*"

class IMenuProfile;

namespace Menu
{
	class CProfile; // See <menu/profile.hpp>.

	using CProfileSystemBase = CSystemBase<CProfile *>;

	class CProfileSystem : public CProfileSystemBase, public IMenuProfileSystem
	{
	public:
		using Base = CProfileSystemBase;

		CProfileSystem();

	public:
		bool Load(const char *pszBaseGameDir, const char *pszPathID, CUtlVector<CUtlString> &vecMessages);
		void Clear();

	protected:
		bool Load(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
		bool Load2(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);

	public:
		CProfile *GetInternal(const char *pszName = MENUPROFILE_DEFAULT_NAME);

	public: // IMenuProfiles
		IMenuProfile *Get(const char *pszName = MENUPROFILE_DEFAULT_NAME) override;
		void AddOrReplaceRef(const char *pszName, IMenuProfile *pData) override;

		CKeyValues3Context *GetEntityKeyValuesAllocator() override;

	protected:
		CUtlSymbolLarge GetSymbol(const char *pszName);
		CUtlSymbolLarge FindSymbol(const char *pszName) const;

	private:
		CUtlSymbolTableLarge_CI m_tableSymbols;
		CUtlMap<CUtlSymbolLarge, CProfile *> m_map;
		CUtlMap<CUtlSymbolLarge, CProfile *> m_mapRefs;

		CKeyValues3Context m_aEntityKeyValuesAllocator;
	};
};

#endif // _INCLUDE_METAMOD_SOURCE_MENU_PROFILE_SYSTEM_HPP_
