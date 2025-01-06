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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_GAME_EVENT_SYSTEM_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_GAME_EVENT_SYSTEM_HPP_

#	pragma once

#	include <functional>
#	include <memory>

#	include <igameevents.h>
#	include <playerslot.h>
#	include <tier1/convar.h>

#	include <logger.hpp>

#	define MENU_SYSTEM_GAME_EVENT_SYSTEM_LOGGINING_COLOR {255, 127, 0, 191}

namespace Menu
{
	class GameEventSystem : public IGameEventListener2, virtual public Logger
	{
	public:
		GameEventSystem();

		using OnCallback_t = std::function<void (IGameEvent *)>;
		using OnCallbackShared_t = std::shared_ptr<OnCallback_t>;

		class SharedCallback
		{
		public:
			SharedCallback()
				:  m_pCallback(std::make_shared<OnCallback_t>(nullptr))
			{
			}

			SharedCallback(const OnCallbackShared_t &funcSharedCallback)
				:  m_pCallback(funcSharedCallback)
			{
			}

			SharedCallback(const OnCallback_t &funcCallback)
				:  m_pCallback(std::make_shared<OnCallback_t>(funcCallback))
			{
			}

			operator OnCallbackShared_t() const
			{
				return m_pCallback;
			}

			operator OnCallback_t() const
			{
				return *m_pCallback;
			}

		private:
			OnCallbackShared_t m_pCallback;
		}; // Menu::GameEventSystem::SharedCallback

	public:
		const char *GetName();

	public:
		bool HookAll();
		bool UnhookAll();

	public:
		bool Register(const char *pszName, const SharedCallback &fnCallback);
		bool Unregister(const char *pszName);
		void UnregisterAll();

	public:
		bool DumpGameEvent(IGameEvent *pEvent);

	protected: // IGameEventListener2
		void FireGameEvent(IGameEvent *pEvent) override;

	protected:
		CUtlSymbolLarge GetSymbol(const char *pszText);
		CUtlSymbolLarge FindSymbol(const char *pszText) const;

	private:
		CUtlSymbolTableLarge_CI m_aSymbolTable;
		CUtlMap<CUtlSymbolLarge, SharedCallback> m_mapCallbacks;

	private: // ConVars
		ConVar<bool> m_aEnableDetaillsConVar;
	}; // Menu::GameEventSystem
}; // Menu

#endif // _INCLUDE_METAMOD_SOURCE_MENU_GAME_EVENT_SYSTEM_HPP_
