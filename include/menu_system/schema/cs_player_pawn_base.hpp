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

#	include <menu_system/schema/base_player_pawn.hpp>
#	include <menu_system/schema.hpp>

#	define CCSPLAYERPAWNBASE_CLASS_NAME "CCSPlayerPawnBase"

class QAngle;
class Vector;
class CPlayer_ViewModelServices;
class CCSPlayer_ViewModelServices;

class CCSPlayerPawnBase : public CBasePlayerPawn
{
};

namespace MenuSystem
{
	namespace Schema
	{
		class CCSPlayerPawnBase_Helper
		{
		public:
			CCSPlayerPawnBase_Helper(CSystem *pSchemaSystemHelper);

		public:
			void Clear();

		public:
			SCHEMA_INSTANCE_ACCESSOR_METHOD(GetViewModelServicesAccessor, CCSPlayerPawnBase, CPlayer_ViewModelServices *, m_aOffsets.m_nViewModelServices);
			SCHEMA_INSTANCE_ACCESSOR_METHOD(GetEyeAnglesAccessor, CCSPlayerPawnBase, QAngle, m_aOffsets.m_nEyeAngles);

		private:
			CSystem::CClass *m_pClass;
			CSystem::CClass::Fields::ListenerCallbacksCollector m_aClassFieldsClassbacks;

			struct
			{
				int m_nViewModelServices = INVALID_SCHEMA_FIELD_OFFSET;
				int m_nEyeAngles = INVALID_SCHEMA_FIELD_OFFSET;
			} m_aOffsets;
		}; // MenuSystem::Schema::CCSPlayerPawnBase_Helper
	}; // MenuSystem::Schema
}; // MenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_CS_PLAYER_PAWN_BASE_HPP_
