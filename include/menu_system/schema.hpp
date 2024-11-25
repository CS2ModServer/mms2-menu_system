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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_HPP_

#	pragma once

#	include <tier0/utlstring.h>
#	include <tier1/utlsymbollarge.h>
#	include <tier1/utlvector.h>
#	include <tier1/utlmap.h>

#	include <gamedata.hpp>

#	define INVALID_SCHEMA_FIELD_OFFSET -1

class ISchemaSystem;
class CSchemaClassInfo;
class CSchemaSystemTypeScope;
class CSchemaType_DeclaredClass;
struct SchemaClassFieldData_t;

namespace MenuSystem
{
	class CSchemaSystem_Helper
	{
	public:
		CSchemaSystem_Helper();

	public:
		bool Init(ISchemaSystem *pSchemaSystem, const CUtlVector<const char *> &vecLoadedLibraries, GameData::CBufferStringVector *pMessages = nullptr);
		void Destroy();

	public:
		class CClass
		{
			friend class CSchemaSystem_Helper;

		public:
			using Fields = GameData::Config::Storage<CUtlSymbolLarge, SchemaClassFieldData_t *>;

			Fields &GetFields();

			SchemaClassFieldData_t *GetField(const CUtlSymbolLarge &sName) const;
			void SetField(const CUtlSymbolLarge &sName, SchemaClassFieldData_t *pData);

			SchemaClassFieldData_t *FindField(const char *pszName) const;
			int FindFieldOffset(const char *pszName) const; // Returns -1 if not found.

		protected:
			void ParseFields(CSchemaClassInfo *pInfo);

		public: // Fields symbols.
			CUtlSymbolLarge GetFieldSymbol(const char *pszName);
			CUtlSymbolLarge FindFieldSymbol(const char *pszName) const;

		private:
			Fields m_aFieldStorage;
			CUtlSymbolTableLarge m_tableFileds;
		};

		CClass *GetClass(const char *pszName);
		CClass *FindClass(const char *pszName);
		int FindClassFieldOffset(const char *pszClassName, const char *pszFiledName); // Returns -1 if not found.

	protected:
		void ParseClasses(CSchemaSystemTypeScope *pType);
		void ClearClasses();

	public: // Class symbols.
		CUtlSymbolLarge GetClassSymbol(const char *pszName);
		CUtlSymbolLarge FindClassSymbol(const char *pszName) const;

	private:
		CUtlVector<CSchemaSystemTypeScope *> m_vecTypeScopes;

		CUtlSymbolTableLarge m_tableClasses;
		CUtlMap<CUtlSymbolLarge, CClass> m_mapClasses;
	}; // CSchemaSystem_Helper
}; // MenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_HPP_
