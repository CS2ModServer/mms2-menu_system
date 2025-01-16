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

#include <menu/pathresolver.hpp>

#include <cstddef>

#include <dynlibutils/module.hpp>

#include <tier0/basetypes.h>
#include <tier0/dbg.h>

Menu::PathResolver::PathResolver(const void *pInitModule)
 :  m_pModule(pInitModule)
{
}

bool Menu::PathResolver::Init()
{
	m_sModuleFilename = DynLibUtils::CModule(m_pModule).GetModulePath();

	return true;
}

void Menu::PathResolver::Clear()
{
	m_sModuleFilename = {};
}

std::string_view Menu::PathResolver::GetAbsoluteModuleFilename()
{
	return m_sModuleFilename;
}

std::string_view Menu::PathResolver::Extract(std::string_view sStartMarker, std::string_view sEndMarker)
{
	auto &sFullPath = m_sModuleFilename;

	std::size_t nStartPosition = sFullPath.find(sStartMarker);

	if(nStartPosition != std::string_view::npos)
	{
		std::size_t nEndPosition = sFullPath.find(sEndMarker, nStartPosition);

		if(nEndPosition != std::string_view::npos)
		{
			return sFullPath.substr(nStartPosition, nEndPosition - (nStartPosition + 1));
		}
	}

	return "";
}
