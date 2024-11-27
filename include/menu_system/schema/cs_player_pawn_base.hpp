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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_CS_PLAYER_PAWN_BASE_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_CS_PLAYER_PAWN_BASE_HPP_

#	pragma once

#	include <menu_system/schema.hpp>

#	include <tier0/dbg.h>
#	include <tier0/platform.h>

#	define CCSPLAYERPAWNBASE_CLASS_NAME "CCSPlayerPawnBase"

class QAngle;
class Vector;
class CCSPlayerPawnBase;

namespace MenuSystem
{
	namespace Schema
	{
		class CCSPlayerPawnBase_Helper
		{
		public:
			CCSPlayerPawnBase_Helper(CSchemaSystem_Helper *pSchemaSystemHelper);

		public:
			void Clear();

		public:
			FORCEINLINE QAngle *GetEyeAngles(CCSPlayerPawnBase *pInstance);

		private:
			CSchemaSystem_Helper::CClass *m_pClass;
			CSchemaSystem_Helper::CClass::Fields::ListenerCallbacksCollector m_aClassFieldsClassbacks;

			struct
			{
				int m_nEyeAngles = INVALID_SCHEMA_FIELD_OFFSET;
			} m_aOffsets;
		}; // MenuSystem::Schema::CBodyComponent_Helper
	}; // MenuSystem::Schema
}; // MenuSystem

FORCEINLINE QAngle *MenuSystem::Schema::CCSPlayerPawnBase_Helper::GetEyeAngles(CCSPlayerPawnBase *pInstance)
{
	Assert(m_aOffsets.m_nEyeAngles != INVALID_SCHEMA_FIELD_OFFSET);

	return (QAngle *)((uintp)pInstance + m_aOffsets.m_nEyeAngles);
}

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_CS_PLAYER_PAWN_BASE_HPP_
