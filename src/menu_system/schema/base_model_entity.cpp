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

#include <menu_system/schema/base_model_entity.hpp>

#include <stdint.h>

#include <schemasystem/schemasystem.h>

MenuSystem::Schema::CBaseModelEntity_Helper::CBaseModelEntity_Helper(CSchemaSystem_Helper *pSchemaSystemHelper)
{
	auto &aCallbacks = m_aClassFieldsClassbacks;

	m_pClass = pSchemaSystemHelper->GetClass(CBASEMODELENTITY_CLASS_NAME);

	Assert(m_pClass);

	auto &aFields = m_pClass->GetFields();

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_vecViewOffset"), [&](const CUtlSymbolLarge &, SchemaClassFieldData_t *pField)
	{
		m_aOffsets.m_nViewOffset = pField->m_nSingleInheritanceOffset;
	});

	aFields.AddListener(&aCallbacks);
}

void MenuSystem::Schema::CBaseModelEntity_Helper::Clear()
{
	m_aOffsets = {};
}

Vector *MenuSystem::Schema::CBaseModelEntity_Helper::GetViewOffset(CBaseModelEntity *pInstance)
{
	Assert(m_aOffsets.m_nViewOffset != INVALID_SCHEMA_FIELD_OFFSET);

	return (Vector *)((uintptr_t)pInstance + m_aOffsets.m_nViewOffset);
}
