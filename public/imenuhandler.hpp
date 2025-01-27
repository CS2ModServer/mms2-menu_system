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


#ifndef _INCLUDE_METAMOD_SOURCE_IMENUHANDLER_HPP_
#define _INCLUDE_METAMOD_SOURCE_IMENUHANDLER_HPP_

#pragma once

#include "imenu.hpp"

#include <basetypes.h>
#include <playerslot.h>

/**
 * @file imenu.hpp
 * @brief Defines an IMenuHandler interface for handling menu-related events.
 */

#define MENU_CONTROL_ITEM_POSITION static_cast<IMenu::ItemPosition_t>(-1)   ///< Menu item position that a control.

/**
 * @brief A Menu Handler interface.
**/
class IMenuHandler
{
public: // The definitions.
	/**
	 * @brief Enumeration of reasons why a menu may end.
	 */
	enum EndReason_t : int8
	{
		MenuEnd_Close = -1,             ///< Closed by a plugin.
		MenuEnd_Selected = 0,           ///< Menu item was selected.
		MenuEnd_Disconnected = 1,       ///< Client dropped from the server.
		MenuEnd_Interrupted = 2,        ///< Client was interrupted by another menu.
		MenuEnd_Exit = 4,               ///< Client selected "exit" on a paginated menu.
		MenuEnd_NoDisplay = 5,          ///< Menu could not be displayed to the client.
		MenuEnd_Timeout = 6,            ///< Menu timed out.
		MenuEnd_ExitBack = 7,           ///< Client selected "exit back" on a paginated menu.
	};

public: // Public methods.
	/** 
	 * @brief Invoked when a menu display/selection cycle begins.
	 *
	 * @param pMenu        A pointer to the menu instance.
	 */
	virtual void OnMenuStart(IMenu *pMenu) {}

	/**
	 * @brief Invoked before a menu is displayed. Allows customization of the menu title.
	 *
	 * @param pMenu         A pointer to the menu instance.
	 * @param aSlot         The client slot.
	 */
	virtual void OnMenuDisplay(IMenu *pMenu, CPlayerSlot aSlot) {}

	/**
	 * @brief Invoked when a menu item is selected.
	 *
	 * @param pMenu         A pointer to the menu instance.
	 * @param aSlot         The client slot.
	 * @param iItem         The item index selected by the client.
	 */
	virtual void OnMenuSelect(IMenu *pMenu, CPlayerSlot aSlot, IMenu::ItemPosition_t iItem) {}

	/**
	 * @brief Invoked when a menu display/selection cycle ends.
	 *
	 * @param pMenu         A pointer to the menu instance.
	 * @param eReason       Reason for the menu ending.
	 */
	virtual void OnMenuEnd(IMenu *pMenu, EndReason_t eReason) {}

	/**
	 * @brief Invoked when the menu object is destroyed.
	 *
	 * @param pMenu         Returns a pointer to the menu instance.
	 */
	virtual void OnMenuDestroy(IMenu *pMenu) {}

	/**
	 * @brief Invoked to determine how a menu title should be rendered.
	 *
	 * @param pMenu         A pointer to the menu instance.
	 * @param aSlot         The client slot.
	 * @param aTitle        The referance to a title to modify.
	 */
	virtual void OnMenuDrawTitle(IMenu *pMenu, CPlayerSlot aSlot, IMenu::Title_t &aTitle) {}

	/**
	 * @brief Invoked to customize the rendering of a specific menu item.
	 *
	 * @param pMenu         A pointer to the menu instance.
	 * @param aSlot         The client slot.
	 * @param iItem         The item index. Can be `ItemControls_t` values.
	 * @param aData         The reference to an item data to modify.
	 */
	virtual void OnMenuDisplayItem(IMenu *pMenu, CPlayerSlot aSlot, IMenu::ItemPosition_t iItem, IMenu::Item_t &aData) {}
};

#endif // _INCLUDE_METAMOD_SOURCE_IMENUHANDLER_HPP_
