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
			Vector *GetAbsOrigin(CGameSceneNode *pInstance);
			QAngle *GetAbsRotation(CGameSceneNode *pInstance);

		private:
			CSchemaSystem_Helper::CClass *m_pClass;
			CSchemaSystem_Helper::CClass::Fields::ListenerCallbacksCollector m_aClassFieldsClassbacks;

			struct
			{
				int m_nAbsOrigin = -1;
				int m_nAbsRotation = -1;
			} m_aOffsets;
		}; // CGameSceneNode_Helper
	}; // Schema
}; // MenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_GAME_SCENE_NODE_HPP_
