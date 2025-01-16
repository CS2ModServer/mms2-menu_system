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

#include <menu/chatcommandsystem.hpp>

#include <tier1/utlrbtree.h>

Menu::ChatCommandSystem::ChatCommandSystem()
 :  Logger(GetName(), NULL, 0, LV_DEFAULT, MENU_CHATCOMMANDSYSTEM_LOGGINING_COLOR), 
    Base()
{
}

const char *Menu::ChatCommandSystem::GetName()
{
	return "Menu - Chat Command System";
}

const char *Menu::ChatCommandSystem::GetHandlerLowercaseName()
{
	return "chat command";
}

char Menu::ChatCommandSystem::GetPublicTrigger()
{
	return '!';
}

char Menu::ChatCommandSystem::GetSilentTrigger()
{
	return '/';
}

bool Menu::ChatCommandSystem::Handle(const char *pszName, CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArgs)
{
	if(aSlot == CPlayerSlot::InvalidIndex())
	{
		Logger::Message("Type the chat command from root console?\n");

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

	return Base::Handle(pszName, aSlot, bIsSilent, vecArgs);
}

