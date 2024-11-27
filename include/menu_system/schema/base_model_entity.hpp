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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_BASE_MODEL_ENTITY_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_BASE_MODEL_ENTITY_HPP_

#	pragma once

#	include <menu_system/schema.hpp>

#	include <tier0/dbg.h>
#	include <tier0/platform.h>

#	define CBASEMODELENTITY_CLASS_NAME "CBaseModelEntity"

class QAngle;
class Vector;
class CBaseModelEntity;

namespace MenuSystem
{
	namespace Schema
	{
		class CBaseModelEntity_Helper
		{
		public:
			CBaseModelEntity_Helper(CSchemaSystem_Helper *pSchemaSystemHelper);

		public:
			void Clear();

		public:
			FORCEINLINE Vector *GetViewOffset(CBaseModelEntity *pInstance);

		private:
			CSchemaSystem_Helper::CClass *m_pClass;
			CSchemaSystem_Helper::CClass::Fields::ListenerCallbacksCollector m_aClassFieldsClassbacks;

			struct
			{
				int m_nViewOffset = INVALID_SCHEMA_FIELD_OFFSET;
			} m_aOffsets;
		}; // MenuSystem::Schema::CBaseModelEntity_Helper
	}; // MenuSystem::Schema
}; // MenuSystem

FORCEINLINE Vector *MenuSystem::Schema::CBaseModelEntity_Helper::GetViewOffset(CBaseModelEntity *pInstance)
{
	Assert(m_aOffsets.m_nViewOffset != INVALID_SCHEMA_FIELD_OFFSET);

	return reinterpret_cast<Vector *>(reinterpret_cast<uintp>(pInstance) + m_aOffsets.m_nViewOffset);
}

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_BASE_MODEL_ENTITY_HPP_
