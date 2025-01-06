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

#include <globals.hpp>
#include <menu/game_event_system.hpp>

#include <tier1/utlrbtree.h>

Menu::GameEventSystem::GameEventSystem()
 :  Logger(GetName(), NULL, 0, LV_DEFAULT, MENU_SYSTEM_GAME_EVENT_SYSTEM_LOGGINING_COLOR), 
    m_mapCallbacks(DefLessFunc(const CUtlSymbolLarge)), 

    m_aEnableDetaillsConVar("mm_" META_PLUGIN_PREFIX "_enable_game_events_details", FCVAR_RELEASE | FCVAR_GAMEDLL, "Enable detail messages of game events", false, true, false, true, true)
{
}

const char *Menu::GameEventSystem::GetName()
{
	return "Menu - Game Event System";
}

bool Menu::GameEventSystem::HookAll()
{
	if(!g_pGameEventManager)
	{
		AssertMsg(0, "Game event manager are not ready!\n");

		return false;
	}

	auto *pEventListener = static_cast<IGameEventListener2 *>(this);

	unsigned int nFails = 0;

	FOR_EACH_MAP_FAST(m_mapCallbacks, i)
	{
		const char *pszName = m_mapCallbacks.Key(i).String();

		if(g_pGameEventManager->AddListener(pEventListener, pszName, true) == -1)
		{
			Logger::WarningFormat("Failed to hook \"%s\" event\n", pszName);

			nFails++;
		}
		else
		{
			if(Logger::IsChannelEnabled(LV_DETAILED))
			{
				Logger::DetailedFormat("Hooked \"%s\" event\n", pszName);
			}
		}
	}

	return nFails != m_mapCallbacks.Count();
}

bool Menu::GameEventSystem::UnhookAll()
{
	if(!g_pGameEventManager)
	{
		AssertMsg(0, "Game event manager are not ready!\n");

		return false;
	}

	g_pGameEventManager->RemoveListener(static_cast<IGameEventListener2 *>(this));

	return true;
}

bool Menu::GameEventSystem::Register(const char *pszName, const SharedCallback &fnCallback)
{
	m_mapCallbacks.Insert(GetSymbol(pszName), fnCallback);

	return true;
}

bool Menu::GameEventSystem::Unregister(const char *pszName)
{
	return m_mapCallbacks.Remove(FindSymbol(pszName));
}

void Menu::GameEventSystem::UnregisterAll()
{
	m_mapCallbacks.Purge();
}

bool Menu::GameEventSystem::DumpGameEvent(IGameEvent *pEvent)
{
	KeyValues3 *pEventDataKeys = pEvent->GetDataKeys();

	if(!pEventDataKeys)
	{
		Logger::WarningFormat("Data keys is empty at \"%s\" event\n", pEvent->GetName());

		return false;
	}

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		int iMemberCount = pEventDataKeys->GetMemberCount();

		if(!iMemberCount)
		{
			Logger::WarningFormat("No members at \"%s\" event\n", pEvent->GetName());

			return false;
		}

		{
			auto aDetails = Logger::CreateDetailsScope();

			aDetails.PushFormat("\"%s\":", pEvent->GetName());
			aDetails.Push("{");

			KV3MemberId_t id = 0;

			do
			{
				const char *pEventMemberName = pEventDataKeys->GetMemberName(id);

				KeyValues3 *pEventMember = pEventDataKeys->GetMember(id);

				CBufferStringGrowable<128> sEventMember;

				pEventMember->ToString(sEventMember, KV3_TO_STRING_DONT_CLEAR_BUFF);
				aDetails.PushFormat("\t\"%s\":\t%s", pEventMemberName, sEventMember.Get());

				id++;
			}
			while(id < iMemberCount);

			aDetails.Push("}");
			aDetails.Send([&](const CUtlString &sMessage)
			{
				Logger::Detailed(sMessage);
			});
		}
	}

	return true;
}

void Menu::GameEventSystem::FireGameEvent(IGameEvent *pEvent)
{
	if(m_aEnableDetaillsConVar.GetValue())
	{
		DumpGameEvent(pEvent);
	}

	const char *pszName = pEvent->GetName();

	CUtlSymbolLarge sName = FindSymbol(pszName);

	if(!sName.IsValid())
	{
		return;
	}

	auto iFound = m_mapCallbacks.Find(sName);

	if(iFound == m_mapCallbacks.InvalidIndex())
	{
		return;
	}

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat(u8"Handling \"%s\" game event…\n", pszName);
	}

	OnCallback_t it = m_mapCallbacks[iFound];

	it(pEvent);
}

CUtlSymbolLarge Menu::GameEventSystem::GetSymbol(const char *pszText)
{
	return m_aSymbolTable.AddString(pszText);
}

CUtlSymbolLarge Menu::GameEventSystem::FindSymbol(const char *pszText) const
{
	return m_aSymbolTable.Find(pszText);
}
