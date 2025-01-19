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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PROFILES_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PROFILES_HPP_

#	include <tier0/platform.h>

namespace Menu
{
	namespace System
	{
		class Profiles
		{

		public:
			bool Init();
			void Clear();

		private:
			CUtlSymbolTableLarge_CI m_aSymbolTable;
			CUtlMap<CUtlSymbolLarge, SharedCallback> m_mapCallbacks;
		};
	};
};

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_PROFILES_HPP_
