
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

#include <menu_system/provider.hpp>

MenuSystem::Provider::GameDataStorage::CGameResource::CGameResource()
{
	{
		auto &aCallbacks = m_aOffsetCallbacks;

		aCallbacks.Insert(m_aGameConfig.GetSymbol("CGameResourceService::m_pEntitySystem"), {[&](const CUtlSymbolLarge &aKey, const ptrdiff_t &nOffset)
		{
			m_nEntitySystemOffset = nOffset;
		}});

		m_aGameConfig.GetOffsets().AddListener(&aCallbacks);
	}
}

bool MenuSystem::Provider::GameDataStorage::CGameResource::Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aGameConfig.Load(pRoot, pGameConfig, vecMessages);
}

void MenuSystem::Provider::GameDataStorage::CGameResource::Reset()
{
	m_nEntitySystemOffset = -1;
}

ptrdiff_t MenuSystem::Provider::GameDataStorage::CGameResource::GetEntitySystemOffset() const
{
	return m_nEntitySystemOffset;
}

