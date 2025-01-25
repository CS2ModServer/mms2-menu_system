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

#ifndef _INCLUDE_METAMOD_SOURCE_IMENUPROFILESYSTEM_HPP_
#define _INCLUDE_METAMOD_SOURCE_IMENUPROFILESYSTEM_HPP_

#pragma once

#define MENUPROFILE_DEFAULT_NAME "default"

class CKeyValues3Context;
class IMenuProfile; // See "imenuprofile.hpp".

/**
 * @brief A Menu Profile System interface.
 */
class IMenuProfileSystem
{
public:
	/**
	 * @brief Retrieves a menu profile by name.
	 * 
	 * @param pszName       Name of the profile. Defaults to "default"
	 * .
	 * @return              Returns a pointer to the menu profile interface.
	 */
	virtual IMenuProfile *Get(const char *pszName = MENUPROFILE_DEFAULT_NAME) = 0;

	/**
	 * @brief Adds or replaces a menu profile.
	 * 
	 * @param pszName       Name of the profile.
	 * @param pData         A pointer to new profile data.
	 */
	virtual void AddOrReplaceRef(const char *pszName, IMenuProfile *pData) = 0;

	/**
	 * @brief Retrieves the entity key values allocator.
	 * 
	 * @return              Returns a pointer to the entity key values allocator.
	 */
	virtual CKeyValues3Context *GetEntityKeyValuesAllocator() = 0;
}; // IMenuProfileSystem

#endif // _INCLUDE_METAMOD_SOURCE_IMENUPROFILESYSTEM_HPP_
