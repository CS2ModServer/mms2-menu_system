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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_BASE_ENTITY_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_BASE_ENTITY_HPP_

#	pragma once

#	include <menu_system/schema.hpp>

#	include <tier0/dbg.h>
#	include <tier0/platform.h>

#	define CBASEENTITY_CLASS_NAME "CBaseEntity"

class QAngle;
class Vector;
class CBaseEntity;
class CBodyComponent;

namespace MenuSystem
{
	namespace Schema
	{
		class CBaseEntity_Helper
		{
		public:
			CBaseEntity_Helper(CSchemaSystem_Helper *pSchemaSystemHelper);

		public:
			void Clear();

		public:
			FORCEINLINE CBodyComponent **GetBodyComponent(CBaseEntity *pInstance);
			FORCEINLINE uint *GetEffects(CBaseEntity *pInstance);
			FORCEINLINE CBaseEntity **GetOwnerEntity(CBaseEntity *pInstance);
			FORCEINLINE int *GetEFlags(CBaseEntity *pInstance);

		private:
			CSchemaSystem_Helper::CClass *m_pClass;
			CSchemaSystem_Helper::CClass::Fields::ListenerCallbacksCollector m_aClassFieldsClassbacks;

			struct
			{
				int m_nBodyComponent = INVALID_SCHEMA_FIELD_OFFSET;
				int m_nEffects = INVALID_SCHEMA_FIELD_OFFSET;
				int m_nOwnerEntity = INVALID_SCHEMA_FIELD_OFFSET;
				int m_nEFlags = INVALID_SCHEMA_FIELD_OFFSET;
			} m_aOffsets;
		}; // MenuSystem::Schema::CBaseEntity_Helper
	}; // MenuSystem::Schema
}; // MenuSystem

FORCEINLINE CBodyComponent **MenuSystem::Schema::CBaseEntity_Helper::GetBodyComponent(CBaseEntity *pInstance)
{
	Assert(m_aOffsets.m_nBodyComponent != INVALID_SCHEMA_FIELD_OFFSET);

	return reinterpret_cast<CBodyComponent **>(reinterpret_cast<uintp>(pInstance) + m_aOffsets.m_nBodyComponent);
}

FORCEINLINE uint *MenuSystem::Schema::CBaseEntity_Helper::GetEffects(CBaseEntity *pInstance)
{
	Assert(m_aOffsets.m_nEffects != INVALID_SCHEMA_FIELD_OFFSET);

	return reinterpret_cast<uint *>(reinterpret_cast<uintp>(pInstance) + m_aOffsets.m_nEffects);
}

FORCEINLINE CBaseEntity **MenuSystem::Schema::CBaseEntity_Helper::GetOwnerEntity(CBaseEntity *pInstance)
{
	Assert(m_aOffsets.m_nOwnerEntity != INVALID_SCHEMA_FIELD_OFFSET);

	return reinterpret_cast<CBaseEntity **>(reinterpret_cast<uintp>(pInstance) + m_aOffsets.m_nOwnerEntity);
}

FORCEINLINE int *MenuSystem::Schema::CBaseEntity_Helper::GetEFlags(CBaseEntity *pInstance)
{
	Assert(m_aOffsets.m_nEFlags != INVALID_SCHEMA_FIELD_OFFSET);

	return reinterpret_cast<int *>(reinterpret_cast<uintp>(pInstance) + m_aOffsets.m_nEFlags);
}

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_BASE_ENTITY_HPP_
