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

#include <menu_system/schema/cs_player_view_model_services.hpp>

#include <schemasystem/schemasystem.h>

MenuSystem::Schema::CCSPlayer_ViewModelServices_Helper::CCSPlayer_ViewModelServices_Helper(CSystem *pSchemaSystemHelper)
{
	auto &aCallbacks = m_aClassFieldsClassbacks;

	m_pClass = pSchemaSystemHelper->GetClass(CCSPLAYER_VIEWMODELSERVICES_CLASS_NAME);

	Assert(m_pClass);

	auto &aFields = m_pClass->GetFields();

	aCallbacks.Insert(m_pClass->GetFieldSymbol("m_hViewModel"), {[&](const CUtlSymbolLarge &, SchemaClassFieldData_t *pField)
	{
		m_aOffsets.m_aViewModel.nValue = pField->m_nSingleInheritanceOffset;

		{
			int nSize {};
			uint8 nAlignment {};

			pField->m_pType->GetSizeAndAlignment(nSize, nAlignment);
			m_aOffsets.m_aViewModel.nArraySize = nSize / sizeof(CHandle<CBaseViewModel>);
		}
	}});

	aFields.AddListener(&aCallbacks);
}

void MenuSystem::Schema::CCSPlayer_ViewModelServices_Helper::Clear()
{
	m_aOffsets = {};
}
