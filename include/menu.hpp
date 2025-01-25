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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_HPP_

#	pragma once

#	include <imenu.hpp>
#	include <imenuhandler.hpp>
#	include "menu/provider.hpp"
#	include "menu/schema/pointworldtext.hpp"

#	include <array>

#	include <basetypes.h>
#	include <bitvec.h>
#	include <const.h>
#	include <tier1/utlvector.h>

#	define MENU_EMPTY_BACKGROUND_MATERIAL_NAME "materials/editor/icon_empty.vmat"

class IMenuHandler;
class IMenuProfile;

struct CMenuData_t
{
	IMenu::Title_t m_title;
	IMenu::Items_t m_vecItems;
	IMenu::ItemControlFlags_t m_eControlFlags;

	struct ControlItems_t
	{
		using ItemPtr_t = IMenu::Item_t *;

		ItemPtr_t m_pBackItem = nullptr;
		ItemPtr_t m_pNextItem = nullptr;
		ItemPtr_t m_pExitItem = nullptr;

		ItemPtr_t Base()
		{
			return m_pBackItem;
		}

		const ItemPtr_t Base() const
		{
			return m_pBackItem;
		}

		constexpr uintp Count() const
		{
			return sizeof(ControlItems_t) / sizeof(ItemPtr_t);
		}

		using iterator = ItemPtr_t;
		using const_iterator = const ItemPtr_t;

		iterator begin()                    { return Base(); }
		const_iterator cbegin() const       { return Base(); }
		iterator end()                      { return Base() + Count(); }
		const_iterator cend() const         { return Base() + Count(); }
	} *m_pControlItems;

	CMenuData_t(ControlItems_t *pControlData = nullptr)
	 :  m_eControlFlags(IMenu::MENU_ITEM_CONTROL_DEFAULT_FLAGS), 
	    m_pControlItems(pControlData)
	{
	}
};

using CMenuBase = CUtlVector<CEntityInstance *>; // List of active menu entities // HL2SDK bcompatibility: CUtlVectorBase<T, I, M>?

class CMenu : public IMenu, public CMenuBase
{
public:
	using CPointWorldText_Helper = Menu::Schema::CPointWorldText_Helper;
	using CGameData_BaseEntity = Menu::CProvider::CGameDataStorage::CBaseEntity;

	CMenu(const CPointWorldText_Helper *pSchemaHelper, const CGameData_BaseEntity *pGameData, IMenuProfile *pProfile, IMenuHandler *pHandler = nullptr, CMenuData_t::ControlItems_t *pControls = nullptr);
	~CMenu() override; // IMenuInstance destructor.

	void Close(IMenuHandler::EndReason_t eReason);
	void Destroy();
	void Purge();

public:	// IMenuInstance
	const IMenuProfile *GetProfile() const override
	{
		return m_pProfile;
	}

	bool ApplyProfile(CPlayerSlot aSlot, IMenuProfile *pNewProfile) override;

	IMenuHandler *GetHandler() const override
	{
		return m_pHandler;
	}

	const CUtlVector<CEntityInstance *> &GetActiveEntities() const override
	{
		return *static_cast<const CUtlVector<CEntityInstance *> *>(this);
	}

	const CPlayerBitVec &GetRecipients() const override
	{
		return m_bvPlayers;
	}

	void Emit(const CUtlVector<CEntityInstance *> &vecEntites) override;

public: // IMenu
	Title_t &GetTitleRef() override
	{
		return m_aData.m_title;
	}

	Items_t &GetItemsRef() override
	{
		return m_aData.m_vecItems;
	}

	ItemControlFlags_t &GetItemControlsRef() override
	{
		return m_aData.m_eControlFlags;
	}

	class CPageBuilder
	{
	public:
		CPageBuilder() = delete;
		CPageBuilder(int nTextSize = MENU_MAX_TEXT_LENGTH);

		void Render(IMenu *pMenu, CMenuData_t &aData, CPlayerSlot aSlot, ItemPosition_t iStartPosition); // Render a page.

		virtual const char *GetBackgroundText() const
		{
			return m_sBackgroundText.Get();
		}

		virtual const char *GetInactiveText() const
		{
			return m_sInactiveText.Get();
		}

		virtual const char *GetActiveText() const
		{
			return m_sActiveText.Get();
		}

	private:
		class CBufferStringText : public CBufferString
		{
		public:
			static const uintp sm_nDataSize = ALIGN_VALUE(MENU_MAX_TEXT_LENGTH - sizeof(char[8]), 8);

			CBufferStringText(int nTextSize)
			 :  CBufferString(nTextSize, false)
			{
			}

		private:
			char m_FixedData[sm_nDataSize];
		};

		CBufferStringText m_sBackgroundText;
		CBufferStringText m_sInactiveText;
		CBufferStringText m_sActiveText;
	};

	CPageBuilder *Render(CPlayerSlot aSlot, ItemPosition_t iStartItem = MENU_FIRST_ITEM_INDEX);
	bool InternalDisplayAt(CPlayerSlot aSlot, ItemPosition_t iStartItem = MENU_FIRST_ITEM_INDEX, bool bSetTextNow = false) override;

	static constexpr uint8 sm_nMaxItemsPerPage = MENU_DEFAULT_ITEMS_COUNT_PER_PAGE - 3;
	virtual bool OnSelect(CPlayerSlot aSlot, int iSelectedItem);

public: // Internal methods.
	CEntityKeyValues *GetAllocatedBackgroundKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator = nullptr); // Must be deleted.
	CEntityKeyValues *GetAllocatedInactiveKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator = nullptr, bool bDrawBackground = true); // Must be deleted.
	CEntityKeyValues *GetAllocatedActiveKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator = nullptr, bool bDrawBackground = true); // Must be deleted.
	CUtlVector<CEntityKeyValues *> GenerateKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator = nullptr, bool bIncludeBackground = true); // Must be closed with `PurgeAndDeleteElements()`.
	static Color CalculatePassiveColor(const Color &rgbaActive, const Color &rgbaInactive);

protected:
	CEntityInstance *GetBackgroundEntity()
	{
		return GetActiveEntities()[MENU_ENTITY_BACKGROUND_INDEX];
	}

	CEntityInstance *GetInactiveEntity()
	{
		return GetActiveEntities()[MENU_ENTITY_INACTIVE_INDEX];
	}

	CEntityInstance *GetActiveEntity()
	{
		return GetActiveEntities()[MENU_ENTITY_ACTIVE_INDEX];
	}

protected:
	void InternalSetMessage(MenuEntity_t eEntity, const char *pszText);
	const CPageBuilder *GetCurrentPage(CPlayerSlot aSlot)
	{
		return m_arrCachedPagesMap[aSlot.GetClientIndex()].Element(m_arrCurrentPositions[aSlot.GetClientIndex()]);
	}

private: // IMenuInstance fields.
	const CPointWorldText_Helper *m_pSchemaHelper_PointWorldText;
	const CGameData_BaseEntity *m_pGameData_BaseEntity;
	IMenuProfile *m_pProfile;
	IMenuHandler *m_pHandler;
	CPlayerBitVec m_bvPlayers;

private: // IMenu fields.
	CMenuData_t m_aData;

protected: // Pages fields.
	std::array<ItemPosition_t, ABSOLUTE_PLAYER_LIMIT> m_arrCurrentPositions; // By slots.
	using ItemPages_t = CUtlMap<ItemPosition_t, CPageBuilder *>;
	std::array<ItemPages_t, ABSOLUTE_PLAYER_LIMIT + 1> m_arrCachedPagesMap; // By client indexes.

	CPageBuilder *m_pCurrentPage = nullptr;
}; // Menu

#endif // _INCLUDE_METAMOD_SOURCE_MENU_HPP_
