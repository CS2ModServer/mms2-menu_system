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


#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_GAME_SCENE_NODE_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_GAME_SCENE_NODE_HPP_

#	pragma once

#	include <menu_system/schema.hpp>

#	include <tier0/dbg.h>
#	include <tier0/platform.h>

#	define CGAMESCENENODE_CLASS_NAME "CGameSceneNode"

class QAngle;
class Vector;
class CGameSceneNode;

namespace MenuSystem
{
	namespace Schema
	{
		class CGameSceneNode_Helper
		{
		public:
			CGameSceneNode_Helper(CSchemaSystem_Helper *pSchemaSystemHelper);

		public:
			void Clear();

		public:
			FORCEINLINE Vector *GetAbsOrigin(CGameSceneNode *pInstance);
			FORCEINLINE QAngle *GetAbsRotation(CGameSceneNode *pInstance);

		private:
			CSchemaSystem_Helper::CClass *m_pClass;
			CSchemaSystem_Helper::CClass::Fields::ListenerCallbacksCollector m_aClassFieldsClassbacks;

			struct
			{
				int m_nAbsOrigin = INVALID_SCHEMA_FIELD_OFFSET;
				int m_nAbsRotation = INVALID_SCHEMA_FIELD_OFFSET;
			} m_aOffsets;
		}; // CGameSceneNode_Helper
	}; // Schema
}; // MenuSystem

FORCEINLINE Vector *MenuSystem::Schema::CGameSceneNode_Helper::GetAbsOrigin(CGameSceneNode *pInstance)
{
	Assert(m_aOffsets.m_nAbsOrigin != INVALID_SCHEMA_FIELD_OFFSET);

	return (Vector *)((uintp)pInstance + m_aOffsets.m_nAbsOrigin);
}

FORCEINLINE QAngle *MenuSystem::Schema::CGameSceneNode_Helper::GetAbsRotation(CGameSceneNode *pInstance)
{
	Assert(m_aOffsets.m_nAbsRotation != INVALID_SCHEMA_FIELD_OFFSET);

	return (QAngle *)((uintp)pInstance + m_aOffsets.m_nAbsRotation);
}

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_GAME_SCENE_NODE_HPP_
