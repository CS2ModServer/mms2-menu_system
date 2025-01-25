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


#ifndef _INCLUDE_METAMOD_SOURCE_IMENUSYSTEM_HPP_
#	define _INCLUDE_METAMOD_SOURCE_IMENUSYSTEM_HPP_

#	pragma once

#	include "imenusystem/isample.hpp"
#	include "imenu.hpp"

#	include <tier1/utlvector.h>

#	define MENUSYSTEM_INTERFACE_NAME "Menu System v1.0.0"
#	define MENU_TIME_FOREVER 0      ///< The menu/panel should be displayed as long as possible.

class CEntityInstance; // See <entity2/entitysystem.h> of Source SDK.
class IMenu; // See "imenu.hpp".
class IMenuProfile; // See "imenuprofile.hpp".
class IMenuProfileSystem; // See "imenuprofilesystem.hpp".
class IMenuHandler; // See "imenuhandler.hpp".

/**
 * @brief A Menu System interface.
 * Note: gets with `ismm->MetaFactory(MENUSYSTEM_INTERFACE_NAME, NULL, NULL);`
**/
class IMenuSystem : public ISample
{
public:
	/**
	 * @brief A player interface.
	**/
	class IPlayer : public IPlayerBase
	{
	public:
		struct MenuData_t
		{
			uint64 m_nEndTimestamp = MENU_TIME_FOREVER;
			IMenu *m_pInstance = nullptr;
		};

		/**
		 * @brief Gets menu entities of the player.
		 * 
		 * @return              A vector of menu entities.
		 */
		virtual CUtlVector<MenuData_t> &GetMenus() = 0;
	};

	/**
	 * @brief Gets a player data.
	 * 
	 * @param aSlot         A player slot.
	 * 
	 * @return              Returns a player data.
	 */
	virtual IPlayer *GetPlayer(const CPlayerSlot &aSlot) = 0;

	/**
	 * @brief Gets a menu profiles.
	 * 
	 * @return              Returns a profiles pointer.
	 */
	virtual IMenuProfileSystem *GetProfiles() = 0;

	/**
	 * @brief Allocates a menu instance. 
	 * NOTE: Must be closed with CloseMenu()!
	 * 
	 * @param pProfile      The profile styles of new menu.
	 * @param pHandler      A menu handler.
	 * 
	 * @return              Returns the allocated menu instance.
	 */
	virtual IMenu *CreateMenu(IMenuProfile *pProfile, IMenuHandler *pHandler = nullptr) = 0;

	/**
	 * @brief Display a menu instance to the player.
	 * 
	 * @param pMenu         The menu instance.
	 * @param aSlot         The player slot.
	 * @param nManyTimes    The display time in seconds (default: MENU_TIME_FOREVER).
	 * 
	 * @return              `true` if the menu was displayed successfully,
	 *                      `false` otherwise.
	 */
	virtual bool DisplayMenuToPlayer(IMenu *pMenu, CPlayerSlot aSlot, IMenu::ItemPosition_t iStartItem = MENU_FIRST_ITEM_INDEX, int nManyTimes = MENU_TIME_FOREVER) = 0;

	/**
	 * @brief Closes a menu instance.
	 * 
	 * @param pMenu         A menu instance to close.
	 * 
	 * @return              
	 */
	virtual bool CloseMenu(IMenu *pMenu) = 0;
}; // IMenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_IMENUSYSTEM_HPP_
