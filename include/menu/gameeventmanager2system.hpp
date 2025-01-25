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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_GAMEEVENTSYSTEM_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_GAMEEVENTSYSTEM_HPP_

#	pragma once

#	include "systembase.hpp"

#	include <igameevents.h>
#	include <playerslot.h>
#	include <tier1/convar.h>

#	include <logger.hpp>

#	define MENU_GAMEEVENTSYSTEM_LOGGINING_COLOR {255, 127, 0, 191}

namespace Menu
{
	using CGameEventSystemBase = CSystemBase<IGameEvent *>;

	class CGameEventManager2System : public IGameEventListener2, public CGameEventSystemBase
	{
	public:
		using Base = CGameEventSystemBase;

	public:
		CGameEventManager2System();

	public:
		const char *GetName() override;
		const char *GetHandlerLowercaseName() override;

	public:
		bool HookAll();
		bool UnhookAll();

	public: // CGameEventSystemBase
		bool Handle(const char *pszName, IGameEvent *pEvent) override;

	protected:
		bool DumpGameEvent(IGameEvent *pEvent);

	private: // IGameEventListener2
		void FireGameEvent(IGameEvent *pEvent) override;

	private: // ConVars
		ConVar<bool> m_aEnableDetaillsConVar;
	}; // Menu::CGameEventManager2System
}; // Menu

#endif // _INCLUDE_METAMOD_SOURCE_MENU_GAMEEVENTSYSTEM_HPP_
