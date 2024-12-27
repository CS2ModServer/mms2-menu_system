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

#include <menu_system/schema/base_entity.hpp>

#include <schemasystem/schemasystem.h>

MenuSystem::Schema::CBaseEntity_Helper::CBaseEntity_Helper(CSystem *pSchemaSystemHelper)
{
	auto &aCallbacks = m_aClassFieldsClassbacks;

	m_pClass = pSchemaSystemHelper->GetClass(CBASEENTITY_CLASS_NAME);

	Assert(m_pClass);

	auto &aFields = m_pClass->GetFields();

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_CBodyComponent"), {[&](const CUtlSymbolLarge &, SchemaClassFieldData_t *pField)
	{
		m_aOffsets.m_nBodyComponent = pField->m_nSingleInheritanceOffset;
	}});

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_fEffects"), {[&](const CUtlSymbolLarge &, SchemaClassFieldData_t *pField)
	{
		m_aOffsets.m_nEffects = pField->m_nSingleInheritanceOffset;
	}});

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_hOwnerEntity"), {[&](const CUtlSymbolLarge &, SchemaClassFieldData_t *pField)
	{
		m_aOffsets.m_nOwnerEntity = pField->m_nSingleInheritanceOffset;
	}});

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_iEFlags"), {[&](const CUtlSymbolLarge &, SchemaClassFieldData_t *pField)
	{
		m_aOffsets.m_nEFlags = pField->m_nSingleInheritanceOffset;
	}});

	aFields.AddListener(&aCallbacks);
}

void MenuSystem::Schema::CBaseEntity_Helper::Clear()
{
	m_aOffsets = {};
}
