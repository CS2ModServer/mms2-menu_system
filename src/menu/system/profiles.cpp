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

#include <menu/system/profiles.hpp>

#include <filesystem.h>
#include <tier1/keyvalues3.h>
#include <tier1/utlrbtree.h>
#include <tier1/utlmap.h>

#include <any_config.hpp>

Menu::CProfile::CProfile::~CProfile()
{
	if(m_pItems)
	{
		delete m_pItems;
	}

	if(m_pMatrixOffset)
	{
		delete m_pMatrixOffset;
	}

	if(m_pInactiveColor)
	{
		delete m_pInactiveColor;
	}

	if(m_pActiveColor)
	{
		delete m_pActiveColor;
	}

	if(m_pData)
	{
		delete m_pData;
	}
}

bool Menu::CProfile::Load(IMenuProfiles *pSystem, KeyValues3 *pData, ProfileLoadFlags_t eFlags, CUtlVector<CUtlString> &vecMessages)
{
	// Metadata_t.
	if(eFlags & PROFILE_LOAD_FLAG_METADATA)
	{
		LoadMetadataBase(pSystem, pData, vecMessages);
	}

	// MenuProfile_t fields.
	m_sDisplayName = pData->GetMemberString("display_name");
	m_sDescription = pData->GetMemberString("description");
	m_pItems = LoadAllocatedItems(pData->FindMember("items"), vecMessages);
	m_sItemsVerificationClientConVarName = pData->GetMemberString("items_verification_client_convar_name");
	m_pMatrixOffset = LoadAllocatedMatrixOffset(pData->FindMember("matrix_offset"), vecMessages);
	m_pInactiveColor = new Color(pData->GetMemberColor("inactive_color"));
	m_pActiveColor = new Color(pData->GetMemberColor("active_color"));
	m_flBackgroundAwayUnits = pData->GetMemberFloat("background_away_units");

	if(!(eFlags & PROFILE_LOAD_FLAG_DONT_REMOVE_STATIC_MEMBERS))
	{
		RemoveStaticMembers(pData);
	}

	if(eFlags & PROFILE_LOAD_ENTITYKEYVALUES)
	{
		m_pData = LoadAllocatedEntityKeyValues(pSystem, pData, vecMessages);
	}

	return true;
}

void Menu::CProfile::LoadMetadataBase(IMenuProfiles *pSystem, KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	// MetadataBase_t fields.
	m_aMetadata.m_bHidden = pData->GetMemberBool("hidden", false);
	LoadMetadataBases(pSystem, "inherits", pData->FindMember("inherits"), vecMessages);
}

bool Menu::CProfile::LoadMetadataBases(IMenuProfiles *pSystem, const char *pszName, KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	if(!pData)
	{
		return false;
	}

	KV3Type_t eMemberType = pData->GetType();

	CBufferStringN<256> sMessage;

	if(eMemberType == KV3_TYPE_STRING)
	{
		if(!LoadMetadataBaseString(pSystem, pszName, pData, sMessage))
		{
			vecMessages.AddToTail(sMessage);
		}
	}
	else if(eMemberType == KV3_TYPE_ARRAY || eMemberType == KV3_TYPE_TABLE)
	{
		FOR_EACH_KV3(pData, it)
		{
			KeyValues3 *pArrayMember = it.Get();

			if(pArrayMember->GetType() == KV3_TYPE_STRING)
			{
				if(!LoadMetadataBaseString(pSystem, pszName, pArrayMember, sMessage))
				{
					vecMessages.AddToTail(sMessage);
				}
			}
			else
			{
				const char *pszMessageConcat[] = {"No handler ", "for \"", pData->GetTypeAsString(), "\" type", "of array by \"", pszName, "\" member"};

				sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
				vecMessages.AddToTail(sMessage);
				sMessage.Clear();
			}
		}
	}
	else
	{
		const char *pszMessageConcat[] = {"No handler ", "for \"", pData->GetTypeAsString(), "\" type", "of \"", pszName, "\" member"};

		sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
		vecMessages.AddToTail(sMessage);
		sMessage.Clear();
	}

	return true;
}

bool Menu::CProfile::LoadMetadataBaseString(IMenuProfiles *pSystem, const char *pszName, KeyValues3 *pData, CBufferString &sMessage)
{
	const char *pszMemberValue = pData->GetString();

	auto *pProfile = pSystem->Get(pszMemberValue);

	if(!pProfile)
	{
		const char *pszMessageConcat[] = {"Not found ", pszMemberValue, " profile ", "for \"", pszName, "\" member"};

		sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);

		return false;
	}

	m_aMetadata.m_vecBases.AddToTail(pProfile);

	return true;
}

Menu::CProfile::MatrixOffset_t *Menu::CProfile::LoadAllocatedMatrixOffset(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	if(!pData)
	{
		return nullptr;
	}

	auto *pResult = new MatrixOffset_t();

	pResult->m_flForward = pData->GetMemberFloat("forward");
	pResult->m_flLeft = pData->GetMemberFloat("left");
	pResult->m_flRight = pData->GetMemberFloat("right");
	pResult->m_flUp = pData->GetMemberFloat("up");

	return pResult;
}

Menu::CProfile::Items_t *Menu::CProfile::LoadAllocatedItems(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	if(!pData)
	{
		return nullptr;
	}

	int nMemberCount = pData->GetMemberCount();

	if(!nMemberCount)
	{
		return nullptr;
	}

	auto *pResult = new Items_t();

	pResult->SetCount(nMemberCount);

	CBufferStringN<256> sMessage;

	KV3MemberId_t i = 0;

	do
	{
		const char *pszMemberName = pData->GetMemberName(i);

		KeyValues3 *pMember = pData->GetMember(i);

		int iItem = V_atoi(pszMemberName);

		if(iItem < nMemberCount)
		{
			pResult->Element(iItem) = pMember->GetString();
		}
		else
		{
			const char *pszMessageConcat[] = {"Item \"", pszMemberName, "\" overflow of "};

			sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);

			CUtlString sItemMessage = sMessage;

			sItemMessage += iItem;
			sItemMessage += " limit";
			vecMessages.AddToTail(sItemMessage);
			sMessage.Clear();
		}

		i++;
	}
	while (i < nMemberCount);

	return pResult;
}

CEntityKeyValues *Menu::CProfile::LoadAllocatedEntityKeyValues(IMenuProfiles *pSystem, KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	int nMemberCount = pData->GetMemberCount();

	if(!nMemberCount)
	{
		return nullptr;
	}

	CEntityKeyValues *pResult = new CEntityKeyValues(pSystem->GetEntityKeyValuesAllocator(), EKV_ALLOCATOR_EXTERNAL);

	KV3MemberId_t i = 0;

	do
	{
		const char *pszMemberName = pData->GetMemberName(i);

		KeyValues3 *pMember = pData->GetMember(i);

		KeyValues3 *pEKVMember = pResult->SetKeyValue({pData->GetMemberHash(i), pszMemberName});

		if(pEKVMember)
		{
			*pEKVMember = *pMember;
		}

		i++;
	}
	while (i < nMemberCount);

	return pResult;
}

void Menu::CProfile::RemoveStaticMembers(KeyValues3 *pData)
{
	RemoveStaticMetadataMembers(pData);

	pData->RemoveMember("display_name");
	pData->RemoveMember("description");

	pData->RemoveMember("items");
	pData->RemoveMember("items_verification_client_convar_name");
	pData->RemoveMember("matrix_offset");
	pData->RemoveMember("inactive_color");
	pData->RemoveMember("active_color");
}

void Menu::CProfile::RemoveStaticMetadataMembers(KeyValues3 *pData)
{
	pData->RemoveMember("hidden");
	pData->RemoveMember("inherits");
}

Menu::System::CProfiles::CProfiles()
 :  m_map(DefLessFunc(const CUtlSymbolLarge)), 
    m_mapRefs(DefLessFunc(const CUtlSymbolLarge))
{
}

bool Menu::System::CProfiles::Load(const char *pszBaseGameDir, const char *pszPathID, CUtlVector<CUtlString> &vecMessages)
{
	CBufferStringN<MAX_PATH> sConfigFile;

	CUtlVector<CUtlString> vecConfigFiles;

	CUtlString sError;

	CBufferStringN<256> sMessage;

	AnyConfig::Anyone aGameConfig;

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sError, NULL, pszPathID}, g_KV3Format_Generic});

	{
		sConfigFile.Insert(0, pszBaseGameDir);
		sConfigFile.Insert(sConfigFile.Length(), CORRECT_PATH_SEPARATOR_S);
		sConfigFile.Insert(sConfigFile.Length(), MENU_SYSTEM_PROFILES_FILENAME);

		const char *pszConfigFile = sConfigFile.Get();

		g_pFullFileSystem->FindFileAbsoluteList(vecConfigFiles, pszConfigFile, pszPathID);

		if(vecConfigFiles.Count() < 1)
		{
			const char *pszMessageConcat[] = {"Failed to ", "find \"", pszConfigFile, "\" file"};

			sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
			vecMessages.AddToTail(sMessage);

			return false;
		}

		aLoadPresets.m_pszFilename = vecConfigFiles[0].Get();

		if(!aGameConfig.Load(aLoadPresets)) // Hot.
		{
			const char *pszMessageConcat[] = {"Failed to ", "load \"", pszConfigFile, "\" file", ": ", sError.Get()};

			sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
			vecMessages.AddToTail(sMessage);

			return false;
		}

		if(!Load(aGameConfig.Get(), vecMessages))
		{
			const char *pszMessageConcat[] = {"Failed to ", "apply \"", pszConfigFile, "\" file", ": ", sError.Get()};

			sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
			vecMessages.AddToTail(sMessage);

			return false;
		}

		// ...
	}

	return true;
}

void Menu::System::CProfiles::Clear()
{
	m_tableSymbols.Purge();
	m_map.PurgeAndDeleteElements();
	m_mapRefs.Purge();

	m_aEntityKeyValuesAllocator.Purge();
}

bool Menu::System::CProfiles::Load(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	int nMemberCount = pData->GetMemberCount();

	if(!nMemberCount)
	{
		vecMessages.AddToTail("Profiles is empty");

		return false;
	}

	CBufferStringN<256> sMessage;

	KV3MemberId_t i = 0;

	do
	{
		const char *pszMemberName = pData->GetMemberName(i);

		KeyValues3 *pMember = pData->GetMember(i);

		KV3Type_t eMemberType = pMember->GetType();

		if(eMemberType == KV3_TYPE_STRING)
		{
			// Handle it in Load2().
		}
		else if(eMemberType == KV3_TYPE_TABLE)
		{
			m_map.Insert(GetSymbol(pszMemberName), new CProfile());
		}
		else
		{
			const char *pszMessageConcat[] = {"No handler ", "of \"", pMember->GetTypeAsString(), "\" type", "for \"", pszMemberName, "\" member"};

			sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
			vecMessages.AddToTail(sMessage);
			sMessage.Clear();
		}

		i++;
	}
	while(i < nMemberCount);

	return Load2(pData, vecMessages);
}

bool Menu::System::CProfiles::Load2(KeyValues3 *pData, CUtlVector<CUtlString> &vecMessages)
{
	CBufferStringN<256> sMessage;

	int nMemberCount = pData->GetMemberCount();

	KV3MemberId_t i = 0;

	do
	{
		const char *pszMemberName = pData->GetMemberName(i);

		KeyValues3 *pMember = pData->GetMember(i);

		KV3Type_t eMemberType = pMember->GetType();

		if(eMemberType == KV3_TYPE_STRING)
		{
			const char *pszMemberValue = pMember->GetString();

			auto *pFoundProfile = GetInternal(pszMemberValue);

			if(pFoundProfile)
			{
				m_mapRefs.Insert(GetSymbol(pszMemberName), pFoundProfile);
			}
			else
			{
				const char *pszMessageConcat[] = {"No found \"", pszMemberValue, "\" profile ", "for \"", pszMemberName, "\" member"};

				sMessage.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
				vecMessages.AddToTail(sMessage);
				sMessage.Clear();
			}
		}
		else if(eMemberType == KV3_TYPE_TABLE)
		{
			auto *pFoundProfile = GetInternal(pszMemberName);

			Assert(pFoundProfile);
			pFoundProfile->Load(this, pMember, PROFILE_LOAD_STEP2, vecMessages);
		}
		else
		{
			AssertMsg2(0, "Second load was found unknown \"%s\" type of \"%s\" member", pMember->GetTypeAsString(), pszMemberName);
		}

		i++;
	}
	while(i < nMemberCount);

	return true;
}

Menu::CProfile *Menu::System::CProfiles::GetInternal(const char *pszName)
{
	CUtlSymbolLarge sFound = FindSymbol(pszName);

	if(!sFound.IsValid())
	{
		return nullptr;
	}

	// m_mapRefs commonly used.
	auto iFound = m_mapRefs.Find(sFound);

	if(iFound == m_mapRefs.InvalidIndex())
	{
		iFound = m_map.Find(sFound);

		if(iFound == m_map.InvalidIndex())
		{
			return nullptr;
		}

		return m_map.Element(iFound);
	}

	return m_mapRefs.Element(iFound);
}

MenuProfile_t *Menu::System::CProfiles::Get(const char *pszName)
{
	return GetInternal(pszName);
}

void Menu::System::CProfiles::AddOrReplaceRef(const char *pszName, MenuProfile_t *pData)
{
	m_mapRefs.InsertOrReplace(GetSymbol(pszName), static_cast<CProfile *>(pData));
}

CKeyValues3Context *Menu::System::CProfiles::GetEntityKeyValuesAllocator()
{
	return &m_aEntityKeyValuesAllocator;
}

CUtlSymbolLarge Menu::System::CProfiles::GetSymbol(const char *pszName)
{
	return m_tableSymbols.AddString(pszName);
}

CUtlSymbolLarge Menu::System::CProfiles::FindSymbol(const char *pszName) const
{
	return m_tableSymbols.Find(pszName);
}
