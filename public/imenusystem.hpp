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

#	include <tier1/utlvector.h>

#	define MENUSYSTEM_INTERFACE_NAME "Menu System v1.0.0"

class CEntityInstance;
class IMenuProfileSystem; // See "imenuprofilesystem.hpp"

/**
 * @brief A Menu System interface.
 * Note: gets with "ismm->MetaFactory(MENUSYSTEM_INTERFACE_NAME, NULL, NULL);"
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
		/**
		 * @brief Gets menu entities of the player.
		 * 
		 * @return              A vector of menu entities.
		 */
		virtual CUtlVector<CEntityInstance *> &GetMenuEntities() = 0;
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
}; // IMenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_IMENUSYSTEM_HPP_
