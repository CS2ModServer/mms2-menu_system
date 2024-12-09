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


#ifndef _INCLUDE_METAMOD_SOURCE_IMENU_HPP_
#	define _INCLUDE_METAMOD_SOURCE_IMENU_HPP_

#	pragma once

#	include "imenu/isample.hpp"

#	define MENU_SYSTEM_INTERFACE_NAME "Menu System v1.0.0"

/**
 * @brief A Menu System interface.
 * Note: gets with "ismm->MetaFactory(MENU_SYSTEM_INTERFACE_NAME, NULL, NULL);"
**/
class IMenuSystem : public ISample
{
public:
}; // IMenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_IMENU_HPP_
