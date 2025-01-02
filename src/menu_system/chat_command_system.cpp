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

#include <menu_system/chat_command_system.hpp>

#include <tier1/utlrbtree.h>

MenuSystem::ChatCommandSystem::ChatCommandSystem()
 :  Logger(GetName(), NULL, 0, LV_DEFAULT, MENU_SYSTEM_CHAT_COMMAND_SYSTEM_LOGGINING_COLOR), 
    m_mapCallbacks(DefLessFunc(const CUtlSymbolLarge))
{
}

const char *MenuSystem::ChatCommandSystem::GetName()
{
	return "MenuSystem - Chat Command System";
}

bool MenuSystem::ChatCommandSystem::Register(const char *pszName, const CollectorChangedSharedCallback &fnCallback)
{
	m_mapCallbacks.Insert(m_aSymbolTable.AddString(pszName), fnCallback);

	return true;
}

bool MenuSystem::ChatCommandSystem::Unregister(const char *pszName)
{
	return m_mapCallbacks.Remove(FindSymbol(pszName));
}

void MenuSystem::ChatCommandSystem::UnregisterAll()
{
	m_mapCallbacks.Purge();
}

char MenuSystem::ChatCommandSystem::GetPublicTrigger()
{
	return '!';
}

char MenuSystem::ChatCommandSystem::GetSilentTrigger()
{
	return '/';
}

bool MenuSystem::ChatCommandSystem::Handle(CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArgs)
{
	if(aSlot == CPlayerSlot::InvalidIndex())
	{
		Message("Type the chat command from root console?\n");

		return false;
	}

	if(!vecArgs.Count())
	{
		if(Logger::IsChannelEnabled(LS_DETAILED))
		{
			Logger::Detailed("Chat command arguments is empty\n");
		}

		return false;
	}

	const char *pszName = vecArgs[0];

	auto iFound = m_mapCallbacks.Find(FindSymbol(pszName));

	if(iFound == m_mapCallbacks.InvalidIndex())
	{
		if(Logger::IsChannelEnabled(LS_DETAILED))
		{
			Logger::DetailedFormat("Can't be found \"%s\" command\n", pszName);
		}

		return false;
	}

	if(Logger::IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat(u8"Handling \"%s\" command…\n", pszName);
	}

	OnCallback_t it = m_mapCallbacks[iFound];

	it(aSlot, bIsSilent, vecArgs);

	return true;
}

CUtlSymbolLarge MenuSystem::ChatCommandSystem::GetSymbol(const char *pszText)
{
	return m_aSymbolTable.AddString(pszText);
}

CUtlSymbolLarge MenuSystem::ChatCommandSystem::FindSymbol(const char *pszText) const
{
	return m_aSymbolTable.Find(pszText);
}
