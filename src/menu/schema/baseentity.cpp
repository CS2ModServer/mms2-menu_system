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

#include <menu/schema/baseentity.hpp>

#include <schemasystem/schemasystem.h>

Menu::Schema::CBaseEntity_Helper::CBaseEntity_Helper(CSystem *pSchemaSystemHelper)
{
	auto &aCallbacks = m_aClassFieldsClassbacks;

	m_pClass = pSchemaSystemHelper->GetClass(CBASEENTITY_CLASS_NAME);
	Assert(m_pClass);

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_CBodyComponent"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nBodyComponent));
	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_iTeamNum"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nTeamNum));
	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_fEffects"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nEffects));
	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_hOwnerEntity"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nOwnerEntity));
	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_iEFlags"), SCHEMA_CLASS_FIELD_SHARED_LAMBDA_CAPTURE(m_aOffsets.m_nEFlags));

	m_pClass->GetFields().AddListener(&aCallbacks);
}

void Menu::Schema::CBaseEntity_Helper::Clear()
{
	m_aOffsets = {};
}
