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

#include <menu.hpp>
#include <menu/schema.hpp>
#include <imenuhandler.hpp>
#include <imenuprofile.hpp>
#include <concat.hpp>
#include <globals.hpp>

#include <utility>

#include <entity2/entitykeyvalues.h>

namespace Menu
{
	namespace Utils
	{
		template<typename T, std::size_t... Seq, typename... Args>
		constexpr std::array<T, sizeof...(Seq)> MakeArraySeq(std::index_sequence<Seq...>, Args &... args)
		{
			return {(static_cast<void>(Seq), T(args...))...};
		}

		template <typename T, std::size_t N, typename... Args>
		constexpr std::array<T, N> MakeArray(Args &... args)
		{
			return MakeArraySeq<T>(std::make_index_sequence<N>(), args...);
		}

	}; // Menu::Utils
}; // Menu


CMenu::CMenu(const CPointWorldText_Helper *pSchemaHelper, const CGameData_BaseEntity *pGameData, IMenuProfile *pProfile, IMenuHandler *pHandler, CMenuData_t::ControlItems_t *pControls)
 :  CMenuBase(MENU_MAX_ENTITIES),

    m_pSchemaHelper_PointWorldText(pSchemaHelper), 
    m_pGameData_BaseEntity(pGameData), 

    m_pProfile(pProfile), 
    m_pHandler(pHandler), 

    m_aData(pControls), 
    m_arrCurrentPositions(), 
    m_arrCachedPagesMap(Menu::Utils::MakeArray<ItemPages_t, ABSOLUTE_PLAYER_LIMIT + 1>(DefLessFunc(const ItemPosition_t)))
{
}

CMenu::~CMenu()
{
	Purge();

	// Let's make it a level higher. See MenuSystem_Plugin.
	// variant_t aEmptyVariant {};

	// for(auto *pEntity : GetActiveEntities())
	// {
	// 	if(pEntity)
	// 	{
	// 		m_pGameData_BaseEntity->AcceptInput(pEntity, "Kill", NULL, NULL, &aEmptyVariant, 0);
	// 	}
	// }
}

void CMenu::Close(IMenuHandler::EndReason_t eReason)
{
	auto *pHandler = GetHandler();

	if(pHandler)
	{
		pHandler->OnMenuEnd(static_cast<IMenu *>(this), eReason);
	}

	Destroy();
}

void CMenu::Destroy()
{
	auto *pHandler = GetHandler();

	if(pHandler)
	{
		pHandler->OnMenuDestroy(static_cast<IMenu *>(this));
	}

	Purge();
}

void CMenu::Purge()
{
	for(auto &pCachedPages : m_arrCachedPagesMap)
	{
		if(pCachedPages.Count())
		{
			pCachedPages.PurgeAndDeleteElements();
		}
	}
}

bool CMenu::ApplyProfile(CPlayerSlot aSlot, IMenuProfile *pNewProfile)
{
	const IMenuProfile *pOldProfile = m_pProfile;

	const auto &vecEntities = GetActiveEntities();

	if(pOldProfile == pNewProfile || !vecEntities.Count())
	{
		return false;
	}

	m_pProfile = pNewProfile;

	auto vecMenuKVs = GenerateKeyValues(aSlot, g_pEntitySystem->GetEntityKeyValuesAllocator());

	FOR_EACH_VEC(vecMenuKVs, i)
	{
		auto *pMenuKV = vecMenuKVs[i];

		if(pMenuKV)
		{
			vecEntities[i]->Spawn(pMenuKV);
		}
	}

	vecMenuKVs.PurgeAndDeleteElements();

	return true;
}

void CMenu::Emit(const CUtlVector<CEntityInstance *> &vecEntites)
{
	CopyArray(vecEntites.Base(), vecEntites.Count());

	auto *pHandler = GetHandler();

	if(pHandler)
	{
		pHandler->OnMenuStart(static_cast<IMenu *>(this));
	}
}

CMenu::CPageBuilder *CMenu::Render(CPlayerSlot aSlot, ItemPosition_t iStartItem)
{
	int iClient = aSlot.GetClientIndex();

	auto iFoundPage = m_arrCachedPagesMap[iClient].Find(iStartItem);

	CPageBuilder *pPage {};

	if(iFoundPage == m_arrCachedPagesMap[iClient].InvalidIndex())
	{
		pPage = new CPageBuilder(m_pSchemaHelper_PointWorldText->GetMessageTextSize());

		if(pPage)
		{
			pPage->Render(static_cast<IMenu *>(this), m_aData, aSlot, iStartItem);
			m_arrCachedPagesMap[iClient].Insert(iStartItem, pPage);
		}
	}
	else
	{
		pPage = m_arrCachedPagesMap[iClient].Element(iFoundPage);
	}

	m_arrCurrentPositions[aSlot] = iStartItem;

	return pPage;
}

bool CMenu::InternalDisplayAt(CPlayerSlot aSlot, ItemPosition_t iStartItem, bool bSetTextNow)
{
	m_bvPlayers.Set(aSlot.Get());

	auto *pPage = Render(aSlot, iStartItem);

	if(bSetTextNow)
	{
		InternalSetMessage(MENU_ENTITY_BACKGROUND_INDEX, pPage->GetBackgroundText());
		InternalSetMessage(MENU_ENTITY_INACTIVE_INDEX, pPage->GetInactiveText());
		InternalSetMessage(MENU_ENTITY_ACTIVE_INDEX, pPage->GetActiveText());
	}

	return true;
}

bool CMenu::OnSelect(CPlayerSlot aSlot, int iSlectedItem)
{
	if(iSlectedItem == -1)
	{
		return false;
	}

	const bool bIsNullableItem = !iSlectedItem, 
	           bIsAboveItem = iSlectedItem > sm_nMaxItemsPerPage;

	const bool bIsControl = bIsNullableItem || bIsAboveItem;

	ItemPosition_t iTargetItem {};

	ItemPosition_t iStartItem = m_arrCurrentPositions[aSlot];

	if(bIsNullableItem)
	{
		iTargetItem = MENU_ITEM_CONTROL_EXIT_INDEX;
	}
	else if(bIsAboveItem)
	{
		iTargetItem = static_cast<ItemControls_t>(-static_cast<ItemPosition_t>(iSlectedItem - sm_nMaxItemsPerPage));
	}
	else
	{
		iTargetItem = iStartItem + iSlectedItem;
	}

	if(bIsControl)
	{
		switch(iTargetItem)
		{
			case MENU_ITEM_CONTROL_BACK_INDEX:
			{
				ItemPosition_t iPrevItems = iStartItem - sm_nMaxItemsPerPage;

				if(iPrevItems >= 0)
				{
					InternalDisplayAt(aSlot, iPrevItems, true);
				}

				break;
			}

			case MENU_ITEM_CONTROL_NEXT_INDEX:
			{
				ItemPosition_t iNextItems = iStartItem + sm_nMaxItemsPerPage;

				if(iNextItems < m_aData.m_vecItems.Count())
				{
					InternalDisplayAt(aSlot, iNextItems, true);
				}

				break;
			}
		}
	}

	auto *pHandler = GetHandler();

	if(pHandler)
	{
		pHandler->OnMenuSelect(static_cast<IMenu *>(this), aSlot, iTargetItem);
	}

	return true;
}

CEntityKeyValues *CMenu::GetAllocatedBackgroundKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator)
{
	const IMenuProfile *pProfile = m_pProfile;

	Assert(pProfile);

	CEntityKeyValues *pMenuKV = pProfile->GetAllocactedEntityKeyValues(pAllocator);

	if(pMenuKV)
	{
		const Color *pInactiveColor = pProfile->GetInactiveColor(), 
		            *pActiveColor = pProfile->GetActiveColor();

		if(pInactiveColor && pActiveColor)
		{
			pMenuKV->SetColor("color", CalculatePassiveColor(*pInactiveColor, *pActiveColor));
		}

		pMenuKV->SetString("message", GetCurrentPage(aSlot)->GetBackgroundText());
	}

	return pMenuKV;
}

CEntityKeyValues *CMenu::GetAllocatedInactiveKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator, bool bDrawBackground)
{
	const IMenuProfile *pProfile = m_pProfile;

	Assert(pProfile);

	CEntityKeyValues *pMenuKV = pProfile->GetAllocactedEntityKeyValues(pAllocator);

	if(pMenuKV)
	{
		const Color *pInactiveColor = pProfile->GetInactiveColor();

		if(pInactiveColor)
		{
			pMenuKV->SetColor("color", *pInactiveColor);
		}

		if(bDrawBackground)
		{
			pMenuKV->SetString("background_material_name", MENU_EMPTY_BACKGROUND_MATERIAL_NAME); // To align with the background.
		}

		pMenuKV->SetString("message", GetCurrentPage(aSlot)->GetInactiveText());
	}

	return pMenuKV;
}

CEntityKeyValues *CMenu::GetAllocatedActiveKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator, bool bDrawBackground)
{
	const IMenuProfile *pProfile = m_pProfile;

	Assert(pProfile);

	CEntityKeyValues *pMenuKV = pProfile->GetAllocactedEntityKeyValues(pAllocator);

	if(pMenuKV)
	{
		const Color *pActiveColor = pProfile->GetActiveColor();

		if(pActiveColor)
		{
			pMenuKV->SetColor("color", *pActiveColor);
		}

		if(bDrawBackground)
		{
			pMenuKV->SetString("background_material_name", MENU_EMPTY_BACKGROUND_MATERIAL_NAME); // To align with the background.
		}

		pMenuKV->SetString("message", GetCurrentPage(aSlot)->GetActiveText());
	}

	return pMenuKV;
}

CUtlVector<CEntityKeyValues *> CMenu::GenerateKeyValues(CPlayerSlot aSlot, CKeyValues3Context *pAllocator, bool bIncludeBackground)
{
	Render(aSlot);

	CUtlVector<CEntityKeyValues *> vecResult(MENU_MAX_ENTITIES);

	vecResult.AddToTail(bIncludeBackground ? GetAllocatedBackgroundKeyValues(aSlot, pAllocator) : nullptr);
	vecResult.AddToTail(GetAllocatedInactiveKeyValues(aSlot, pAllocator, bIncludeBackground));
	vecResult.AddToTail(GetAllocatedActiveKeyValues(aSlot, pAllocator, bIncludeBackground));

	return vecResult;
}

Color CMenu::CalculatePassiveColor(const Color &rgbaActive, const Color &rgbaInactive)
{
	struct BackgroundColor_t
	{
		uint16 _r_sum;
		uint16 _g_sum;
		uint16 _b_sum;
		uint16 _a_sum;

		BackgroundColor_t() = delete;
		inline BackgroundColor_t(const Color &left, const Color &right)
		 :  _r_sum(left.r() + right.r()),
		    _g_sum(left.g() + right.g()),
		    _b_sum(left.b() + right.b()),
		    _a_sum(left.a() + right.a())
		{
		}

		inline Color GetPassiveColor()
		{
			return {_r_sum / 3, _g_sum / 3, _b_sum / 3, _a_sum / 3};
		}
	};

	return BackgroundColor_t(rgbaActive, rgbaInactive).GetPassiveColor();
}

void CMenu::InternalSetMessage(MenuEntity_t eEntity, const char *pszText)
{
	Assert(0 <= eEntity && eEntity < Count());

	auto *pEntity = Element(eEntity);

	auto *pPointWorldTextEntity = entity_upper_cast<CPointWorldText *>(pEntity);

	auto aTextAccessor = m_pSchemaHelper_PointWorldText->GetMessageTextAccessor(pPointWorldTextEntity);

	V_strncpy(aTextAccessor, pszText, aTextAccessor.GetSize());
	aTextAccessor.MarkNetworkChanged();
}

CMenu::CPageBuilder::CPageBuilder(int nTextSize)
 :  m_sBackgroundText(nTextSize),
    m_sInactiveText(nTextSize),
    m_sActiveText(nTextSize)
{
	Assert(nTextSize >= MENU_MAX_TEXT_LENGTH);
}

void CMenu::CPageBuilder::Render(IMenu *pMenu, CMenuData_t &aData, CPlayerSlot aSlot, ItemPosition_t iStartPosition)
{
	const auto &aConcat = g_aMenuConcat;

	IMenuHandler *pHandler = pMenu->GetHandler();

	// Append a title.
	{
		auto aTitle = aData.m_title;

		if(pHandler)
		{
			pHandler->OnMenuDrawTitle(static_cast<IMenu *>(pMenu), aSlot, aTitle);
		}

		const auto &aTitleText = aTitle.m_sText;

		if(!aTitleText.IsEmpty())
		{
			const char *pszTitleText = aTitleText.Get();
			
			aConcat.AppendToBuffer(m_sBackgroundText, pszTitleText);
			aConcat.AppendEndsToBuffer(m_sBackgroundText);
			aConcat.AppendToBuffer(m_sInactiveText, pszTitleText);
			aConcat.AppendEndsToBuffer(m_sInactiveText);
			aConcat.AppendEndsAndStartsToBuffer(m_sActiveText);
		}
	}

	// Append items.
	{
		const auto &vecItems = aData.m_vecItems;

		auto eControlFlags = aData.m_eControlFlags;

		bool bHasBackButton = !!(eControlFlags & MENU_ITEM_CONTROL_FLAG_BACK), 
		     bHasNextButton = !!(eControlFlags & MENU_ITEM_CONTROL_FLAG_NEXT), 
		     bHasExitButton = !!(eControlFlags & MENU_ITEM_CONTROL_FLAG_EXIT);

		uint8 nControlsSum = bHasBackButton + bHasNextButton + bHasExitButton;

		int nLeftItems = vecItems.Count() - iStartPosition;

		bool bItemsOverflow = nLeftItems >= sm_nMaxItemsPerPage, // If are elements after.
		     bItemsHasLeft = iStartPosition >= sm_nMaxItemsPerPage; // If are elements behind.

		const ItemPosition_t nItemsOnPage = bItemsOverflow ? (iStartPosition + sm_nMaxItemsPerPage) : vecItems.Count();

		char szItemNumber[2] = ""; // Displayed item number.

		for(ItemPosition_t i = iStartPosition; i < nItemsOnPage; i++)
		{
			auto aItemCopy = vecItems[i];

			if(pHandler)
			{
				pHandler->OnMenuDisplayItem(static_cast<IMenu *>(pMenu), aSlot, i, aItemCopy);
			}

			const auto &sItemContent = aItemCopy.m_sContent;

			if(sItemContent.IsEmpty())
			{
				continue;
			}

			auto eItemStyle = aItemCopy.m_eStyle;

			const char *pszItemContent = sItemContent.Get();

			if(eItemStyle & MENU_ITEM_HASNUMBER)
			{
				szItemNumber[0] = '1' + (i - iStartPosition);
				aConcat.AppendToBuffer(m_sBackgroundText, szItemNumber, pszItemContent);

				if(eItemStyle & MENU_ITEM_ACTIVE)
				{
					aConcat.AppendEndsToBuffer(m_sInactiveText);
					aConcat.AppendToBuffer(m_sActiveText, szItemNumber, pszItemContent);
				}
				else
				{
					aConcat.AppendToBuffer(m_sInactiveText, szItemNumber, pszItemContent);
					aConcat.AppendEndsToBuffer(m_sActiveText);
				}
			}
			else
			{
				aConcat.AppendToBuffer(m_sBackgroundText, pszItemContent);

				if(eItemStyle & MENU_ITEM_ACTIVE)
				{
					aConcat.AppendEndsToBuffer(m_sInactiveText);
					aConcat.AppendToBuffer(m_sActiveText, pszItemContent);
				}
				else
				{
					aConcat.AppendToBuffer(m_sInactiveText, pszItemContent);
					aConcat.AppendEndsToBuffer(m_sActiveText);
				}
			}
		}

		// Append control items.
		auto *pControlItems = aData.m_pControlItems;

		if(nControlsSum && pControlItems)
		{
			aConcat.AppendEndsToBuffer(m_sBackgroundText);
			aConcat.AppendEndsToBuffer(m_sInactiveText);
			aConcat.AppendEndsToBuffer(m_sActiveText);

			auto aControlItems = *pControlItems;

			for(const auto &it : aControlItems)
			{
				auto i = &it - aControlItems.cbegin();

				ItemControls_t eControlItem = static_cast<ItemControls_t>(-static_cast<ItemPosition_t>(i + 1));

				bool bSkipControlItem = (eControlItem == MENU_ITEM_CONTROL_BACK_INDEX && (!bHasBackButton || !bItemsHasLeft)) || 
				                        (eControlItem == MENU_ITEM_CONTROL_NEXT_INDEX && (!bHasNextButton || !bItemsOverflow)) || 
				                        (eControlItem == MENU_ITEM_CONTROL_EXIT_INDEX && (!bHasExitButton)); // Save the margins as in SourceMod.

				auto aItemCopy = it;

				if(pHandler)
				{
					pHandler->OnMenuDisplayItem(static_cast<IMenu *>(pMenu), aSlot, eControlItem, aItemCopy);
				}

				const auto &sItemContent = aItemCopy.m_sContent;

				if(sItemContent.IsEmpty())
				{
					continue;
				}

				auto eItemStyle = aItemCopy.m_eStyle;

				const char *pszItemContent = sItemContent.Get();

				if(eItemStyle & MENU_ITEM_HASNUMBER)
				{
					if(bSkipControlItem)
					{
						aConcat.AppendEndsToBuffer(m_sBackgroundText);
					}
					else
					{
						szItemNumber[0] = '8' + i;

						if(szItemNumber[0] >= ':') // Reset the 10th elements to "0".
						{
							szItemNumber[0] -= 10;
						}

						aConcat.AppendToBuffer(m_sBackgroundText, szItemNumber, pszItemContent);
					}

					if(eItemStyle & MENU_ITEM_ACTIVE)
					{
						aConcat.AppendEndsToBuffer(m_sInactiveText);

						if(bSkipControlItem)
						{
							aConcat.AppendEndsToBuffer(m_sActiveText);
						}
						else
						{
							aConcat.AppendToBuffer(m_sActiveText, szItemNumber, pszItemContent);
						}
					}
					else
					{
						if(bSkipControlItem)
						{
							aConcat.AppendToBuffer(m_sInactiveText, szItemNumber, pszItemContent);
						}
						else
						{
							aConcat.AppendEndsToBuffer(m_sInactiveText);
						}

						aConcat.AppendEndsToBuffer(m_sActiveText);
					}
				}
				else
				{
					aConcat.AppendToBuffer(m_sBackgroundText, pszItemContent);

					if(eItemStyle & MENU_ITEM_ACTIVE)
					{
						aConcat.AppendEndsToBuffer(m_sInactiveText);

						if(bSkipControlItem)
						{
							aConcat.AppendEndsToBuffer(m_sActiveText);
						}
						else
						{
							aConcat.AppendToBuffer(m_sActiveText, pszItemContent);
						}
					}
					else
					{
						if(bSkipControlItem)
						{
							aConcat.AppendEndsToBuffer(m_sInactiveText);
						}
						else
						{
							aConcat.AppendToBuffer(m_sInactiveText, pszItemContent);
						}

						aConcat.AppendEndsToBuffer(m_sActiveText);
					}
				}
			}
		}
	}
}
