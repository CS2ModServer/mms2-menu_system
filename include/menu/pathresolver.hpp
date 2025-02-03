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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_PATHRESOLVER_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_PATHRESOLVER_HPP_

#	define MENU_PATHRESOLVER_ADDONS_DIR "addons"
#	define MENU_PATHRESOLVER_BINARY_DIR "bin"

#	include <dynlibutils/module.hpp>

#	include <stddef.h>

#	include <string_view>

namespace Menu
{
	class CPathResolver
	{
	public:
		CPathResolver(const void *pInitModule);

	public:
		bool Init();
		void Clear();

	public:
		std::string_view GetAbsoluteModuleFilename();
		std::string_view Extract(std::string_view sStartMarker = MENU_PATHRESOLVER_ADDONS_DIR, std::string_view sEndMarker = MENU_PATHRESOLVER_BINARY_DIR);

	private:
		const void *m_pModule;
		DynLibUtils::CModule m_aModule;
	};
};

#endif //_INCLUDE_METAMOD_SOURCE_MENU_PATHRESOLVER_HPP_
