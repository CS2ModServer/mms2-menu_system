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

#include <menu/schema.hpp>

#include <tier0/bufferstring.h>
#include <tier0/commonmacros.h>
#include <tier1/utlmap.h>
#include <schemasystem/schemasystem.h>

Menu::Schema::CSystem::CSystem()
 :  m_mapClasses(DefLessFunc(CUtlSymbolLarge))
{
}

bool Menu::Schema::CSystem::Init(ISchemaSystem *pSchemaSystem, const CUtlVector<const char *> &vecLoadedLibraries, GameData::CBufferStringVector *pMessages)
{
	CBufferStringGrowable<1024> sBuffer;

	for(const auto &pszName : vecLoadedLibraries)
	{
		auto *pTypeScope = pSchemaSystem->FindTypeScopeForModule(pszName);

		if(pTypeScope)
		{
			ParseClasses(pTypeScope);
			m_vecTypeScopes.AddToTail(pTypeScope);
		}
		else if(pMessages)
		{
			const char *pszMessageConcat[] = {"Failed to ", "find \"", pszName, "\" library"};

			pMessages->AddToTail(pszMessageConcat);
		}
	}

	return true;
}

void Menu::Schema::CSystem::Destroy()
{
	ClearClasses();
	m_vecTypeScopes.Purge();
}

Menu::Schema::CSystem::CClass::Fields &Menu::Schema::CSystem::CClass::GetFields()
{
	return m_aFieldStorage;
}

SchemaClassFieldData_t *Menu::Schema::CSystem::CClass::GetField(const CUtlSymbolLarge &sName) const
{
	return m_aFieldStorage.Get(sName);
}

void Menu::Schema::CSystem::CClass::SetField(const CUtlSymbolLarge &sName, SchemaClassFieldData_t *pData)
{
	m_aFieldStorage.Set(sName, pData);
}

SchemaClassFieldData_t *Menu::Schema::CSystem::CClass::FindField(const char *pszName) const
{
	auto sFoundSymbol = FindFieldSymbol(pszName);

	if(!sFoundSymbol.IsValid())
	{
		return nullptr;
	}

	return GetField(sFoundSymbol);
}

int Menu::Schema::CSystem::CClass::FindFieldOffset(const char *pszName) const
{
	auto *pField = FindField(pszName);

	if(!pField)
	{
		return INVALID_SCHEMA_FIELD_OFFSET;
	}

	return pField->m_nSingleInheritanceOffset;
}

void Menu::Schema::CSystem::CClass::ParseFields(CSchemaClassInfo *pInfo)
{
	const uint16 nFieldCount = pInfo->m_nFieldCount;

	for(uint16 n = 0; n < nFieldCount; n++)
	{
		auto &aField = pInfo->m_pFields[n];

		const auto &sField = FindFieldSymbol(aField.m_pszName);

		if(!sField.IsValid())
		{
			continue;
		}

		SetField(sField, &aField);
	}
}

CUtlSymbolLarge Menu::Schema::CSystem::CClass::GetFieldSymbol(const char *pszName)
{
	return m_tableFileds.AddString(pszName);
}

CUtlSymbolLarge Menu::Schema::CSystem::CClass::FindFieldSymbol(const char *pszName) const
{
	return m_tableFileds.Find(pszName);
}

Menu::Schema::CSystem::CClass *Menu::Schema::CSystem::GetClass(const char *pszName)
{
	auto sSymbol = GetClassSymbol(pszName);

	if(!sSymbol.IsValid())
	{
		return nullptr;
	}

	auto iFound = m_mapClasses.Find(sSymbol);

	if(!m_mapClasses.IsValidIndex(iFound))
	{
		iFound = m_mapClasses.Insert(sSymbol, {});
	}

	return &m_mapClasses[iFound];
}

Menu::Schema::CSystem::CClass *Menu::Schema::CSystem::FindClass(const char *pszName)
{
	auto sFoundSymbol = FindClassSymbol(pszName);

	if(!sFoundSymbol.IsValid())
	{
		return nullptr;
	}

	auto iFound = m_mapClasses.Find(sFoundSymbol);

	if(!m_mapClasses.IsValidIndex(iFound))
	{
		return nullptr;
	}

	return &m_mapClasses[iFound];
}

int Menu::Schema::CSystem::FindClassFieldOffset(const char *pszClassName, const char *pszFiledName)
{
	auto *pClass = FindClass(pszClassName);

	if(!pClass)
	{
		return INVALID_SCHEMA_FIELD_OFFSET;
	}

	return pClass->FindFieldOffset(pszFiledName);
}

void Menu::Schema::CSystem::ParseClasses(CSchemaSystemTypeScope *pType)
{
	auto mapDeclaredClasses = pType->m_DeclaredClasses.m_Map;

	FOR_EACH_MAP_FAST(mapDeclaredClasses, i)
	{
		auto *pDeclared = mapDeclaredClasses[i];

		auto *pClassInfo = pDeclared->m_pClassInfo;

		const char *pszClassName = pClassInfo->m_pszName;

		auto *pClass = FindClass(pszClassName);

		if(pClass)
		{
			pClass->ParseFields(pClassInfo);
		}
	}
}

void Menu::Schema::CSystem::ClearClasses()
{
	m_tableClasses.Purge();
	m_mapClasses.Purge();
}

CUtlSymbolLarge Menu::Schema::CSystem::GetClassSymbol(const char *pszName)
{
	return m_tableClasses.AddString(pszName);
}

CUtlSymbolLarge Menu::Schema::CSystem::FindClassSymbol(const char *pszName) const
{
	return m_tableClasses.Find(pszName);
}
