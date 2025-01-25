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

#include <menu/schema/csplayerpawnbase.hpp>

#include <schemasystem/schemasystem.h>

Menu::Schema::CCSPlayerPawnBase_Helper::CCSPlayerPawnBase_Helper(CSystem *pSchemaSystemHelper)
 :  CBaseEntity_Helper(pSchemaSystemHelper), 
    CBaseModelEntity_Helper(pSchemaSystemHelper)
{
	auto &aCallbacks = m_aClassFieldsClassbacks;

	m_pClass = pSchemaSystemHelper->GetClass(CCSPLAYERPAWNBASE_CLASS_NAME);
	Assert(m_pClass);

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_pViewModelServices"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nViewModelServices));
	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_pCameraServices"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nCameraServices));
	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_angEyeAngles"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nEyeAngles));

	m_pClass->GetFields().AddListener(&aCallbacks);
}

void Menu::Schema::CCSPlayerPawnBase_Helper::Clear()
{
	m_aOffsets = {};
}
