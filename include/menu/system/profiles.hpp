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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PROFILES_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PROFILES_HPP_

#	include <imenuprofiles.hpp>
#	include <menu/system.hpp>

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

namespace Menu
{
	enum ProfileLoadFlags_t : uint8
	{
		PROFILE_LOAD_NONE_FLAGS = 0,
		PROFILE_LOAD_FLAG_METADATA = (1 << 0),
		PROFILE_LOAD_FLAG_DONT_REMOVE_STATIC_MEMBERS = (1 << 1),
		PROFILE_LOAD_ENTITYKEYVALUES = (1 << 2),

		PROFILE_LOAD_STEP1 = PROFILE_LOAD_FLAG_DONT_REMOVE_STATIC_MEMBERS,
		PROFILE_LOAD_STEP2 = (PROFILE_LOAD_FLAG_METADATA | PROFILE_LOAD_ENTITYKEYVALUES),
	};

	class CProfile : public MenuProfile_t
	{
	public:
		~CProfile();

		using Base_t = MenuProfile_t;

	public:
		bool Load(IMenuProfiles *pSystem, KeyValues3 *pData, ProfileLoadFlags_t eFlags, CUtlVector<CUtlString> &vecMessages);

	protected:
		void LoadMetadataBase(IMenuProfiles *pSystem, KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
		bool LoadMetadataBases(IMenuProfiles *pSystem, const char *pszName, KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
		bool LoadMetadataBaseString(IMenuProfiles *pSystem, const char *pszName, KeyValues3 *pData, CBufferString &sMessage);

		static Items_t *LoadAllocatedItems(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
		static MatrixOffset_t *LoadAllocatedMatrixOffset(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
		static MatrixOffset_t *LoadAllocatedMatrixOffset2(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
		static CEntityKeyValues *LoadAllocatedEntityKeyValues(IMenuProfiles *pSystem, KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);

	protected:
		static void RemoveStaticMembers(KeyValues3 *pData);
		static void RemoveStaticMetadataMembers(KeyValues3 *pData);
	};

	namespace System
	{
		class CProfiles : public IMenuProfiles
		{
		public:
			CProfiles();

		public:
			bool Load(const char *pszBaseGameDir, const char *pszPathID, CUtlVector<CUtlString> &vecMessages);
			void Clear();

		protected:
			bool Load(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);
			bool Load2(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages);

			CProfile *GetInternal(const char *pszName = MENUPROFILES_DEFAULT_NAME);

		public: // IMenuProfiles
			MenuProfile_t *Get(const char *pszName = MENUPROFILES_DEFAULT_NAME) override;
			void AddOrReplaceRef(const char *pszName, MenuProfile_t *pData) override;

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
};

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PROFILES_HPP_
