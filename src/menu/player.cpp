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

#include <menusystem_plugin.hpp>

#include <serversideclient.h>

MenuSystem_Plugin::CPlayer::CPlayer()
 :  m_pServerSideClient(nullptr), 
    m_vecMenuEntities(), 
    m_pLanguage(nullptr), 
    m_aYourArgumentPhrase({nullptr, nullptr})
{
}

bool MenuSystem_Plugin::CPlayer::AddLanguageListener(IPlayerLanguageListener *pListener)
{
	int iFound = m_vecLanguageCallbacks.Find(pListener);

	bool bIsExists = m_vecLanguageCallbacks.IsValidIndex(iFound);

	if(bIsExists)
	{
		m_vecLanguageCallbacks.AddToTail(pListener);
	}

	return bIsExists;
}

bool MenuSystem_Plugin::CPlayer::RemoveLanguageListener(IPlayerLanguageListener *pListener)
{
	return m_vecLanguageCallbacks.FindAndRemove(pListener);
}

const IMenuSystem::ILanguage *MenuSystem_Plugin::CPlayer::GetLanguage() const
{
	return m_pLanguage;
}

void MenuSystem_Plugin::CPlayer::SetLanguage(const ILanguage *pData)
{
	m_pLanguage = pData;
}

bool MenuSystem_Plugin::CPlayer::IsConnected()
{
	return m_pServerSideClient != nullptr;
}

CServerSideClient *MenuSystem_Plugin::CPlayer::GetServerSideClient()
{
	return m_pServerSideClient;
}

CUtlVector<CEntityInstance *> &MenuSystem_Plugin::CPlayer::GetMenuEntities()
{
	return m_vecMenuEntities;
}

void MenuSystem_Plugin::CPlayer::OnConnected(CServerSideClient *pClient)
{
	m_pServerSideClient = pClient;
}

void MenuSystem_Plugin::CPlayer::OnDisconnected(CServerSideClient *pClient, ENetworkDisconnectionReason eReason)
{
	m_pServerSideClient = nullptr;
	m_vecMenuEntities.Purge();

	m_pLanguage = nullptr;
	m_aYourArgumentPhrase = {};
}

void MenuSystem_Plugin::CPlayer::OnLanguageChanged(CPlayerSlot aSlot, CLanguage *pData)
{
	SetLanguage(pData);

	for(const auto &it : m_vecLanguageCallbacks)
	{
		it->OnPlayerLanguageChanged(aSlot, pData);
	}
}

void MenuSystem_Plugin::CPlayer::TranslatePhrases(const Translations *pTranslations, const CLanguage &aServerLanguage, CUtlVector<CUtlString> &vecMessages)
{
	const struct
	{
		const char *pszName;
		TranslatedPhrase_t *pTranslated;
	} aPhrases[] =
	{
		{
			"Your argument",
			&m_aYourArgumentPhrase,
		}
	};

	const Translations::CPhrase::CContent *paContent;

	Translations::CPhrase::CFormat aFormat;

	int iFound {};

	const auto *pLanguage = GetLanguage();

	const char *pszServerContryCode = aServerLanguage.GetCountryCode(), 
	           *pszContryCode = pLanguage ? pLanguage->GetCountryCode() : pszServerContryCode;

	for(const auto &aPhrase : aPhrases)
	{
		const char *pszPhraseName = aPhrase.pszName;

		if(pTranslations->FindPhrase(pszPhraseName, iFound))
		{
			const auto &aTranslationsPhrase = pTranslations->GetPhrase(iFound);

			if(!aTranslationsPhrase.Find(pszContryCode, paContent) && !aTranslationsPhrase.Find(pszServerContryCode, paContent))
			{
				CUtlString sMessage;

				sMessage.Format("Not found \"%s\" country code for \"%s\" phrase\n", pszContryCode, pszPhraseName);
				vecMessages.AddToTail(sMessage);

				continue;
			}

			aPhrase.pTranslated->m_pFormat = &aTranslationsPhrase.GetFormat();
		}
		else
		{
			CUtlString sMessage;

			sMessage.Format("Not found \"%s\" phrase\n", pszPhraseName);
			vecMessages.AddToTail(sMessage);

			continue;
		}

		if(!paContent->IsEmpty())
		{
			aPhrase.pTranslated->m_pContent = paContent;
		}
	}
}

const MenuSystem_Plugin::CPlayer::TranslatedPhrase_t &MenuSystem_Plugin::CPlayer::GetYourArgumentPhrase() const
{
	return m_aYourArgumentPhrase;
}

const IMenuSystem::ILanguage *MenuSystem_Plugin::GetServerLanguage() const
{
	return &m_aServerLanguage;
}
