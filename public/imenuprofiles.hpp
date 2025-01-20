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


#ifndef _INCLUDE_METAMOD_SOURCE_IMENUPROFILES_HPP_
#	define _INCLUDE_METAMOD_SOURCE_IMENUPROFILES_HPP_

#	pragma once

#	include <entity2/entitykeyvalues.h>
#	include <tier0/basetypes.h>
#	include <tier0/dbg.h>
#	include <tier0/utlstring.h>
#	include <tier1/utlmap.h>
#	include <tier1/utlsymbollarge.h>
#	include <tier1/utlvector.h>

#	include "imenuprofiles/macros.hpp"

#	define MENUPROFILES_DEFAULT_NAME "default"


class Color;
class CKeyValues3Context;
class CEntityInstance;

struct MenuProfile_t
{
	template<class T>
	struct MetadataBase_t
	{
		bool m_bHidden = false; // "hidden"

		// "inherits"
		using Bases_t = CUtlVector<const T *>;
		Bases_t m_vecBases;

		bool IsHidden() const
		{
			return m_bHidden;
		}

		const Bases_t GetBases() const
		{
			return m_vecBases;
		}

		Bases_t GetBaseline() const
		{
			Bases_t vecResult;

			for(const auto &pBase : m_vecBases)
			{
				Assert(pBase);
				vecResult.AddToTail(pBase);

				Bases_t vecSubresult = pBase->GetMetadata().GetBaseline();

				vecResult.AddMultipleToTail(vecSubresult.Count(), vecSubresult.Base());
			}

			return vecResult;
		}
	};

	using Metadata_t = MetadataBase_t<MenuProfile_t>;
	Metadata_t m_aMetadata;

	CUtlString m_sDisplayName; // "display_name"
	CUtlString m_sDescription; // "description"

	using ItemKeyBind_t = CUtlString;
	using Items_t = CUtlVector<ItemKeyBind_t>;
	Items_t *m_pItems = nullptr; // "items"
	CUtlString m_sItemsVerificationClientConVarName; // "items_verification_client_convar_name"

	struct MatrixOffset_t
	{
		vec_t m_flForward = 0.f; // "forward"
		vec_t m_flLeft = 0.f; // "left"
		vec_t m_flRight = 0.f; // "right"
		vec_t m_flUp = 0.f; // "up"
	} *m_pMatrixOffset = nullptr; // "matrix_offset"

	Color *m_pInactiveColor = nullptr; // "inactive_color"
	Color *m_pActiveColor = nullptr; // "active_color"

	float m_flBackgroundAwayUnits = 0.f; // "background_away_units"

	CEntityKeyValues *m_pData = nullptr; // Other elements.

	const Metadata_t &GetMetadata() const
	{
		return m_aMetadata;
	}

	// Base of profile.
	MENUPROFILES_GET_INHERITED_STRING_METHOD(GetDisplayName, m_sDisplayName)
	MENUPROFILES_GET_INHERITED_STRING_METHOD(GetDescription, m_sDescription)

	// Items.
	MENUPROFILES_GET_INHERITED_POINTER_METHOD(GetItems, m_pItems)
	MENUPROFILES_GET_INHERITED_STRING_METHOD(GetItemsVerificationClientConVarName, m_sItemsVerificationClientConVarName)

	// Matrix ones.
	MENUPROFILES_GET_INHERITED_POINTER_METHOD(GetMatrixOffset, m_pMatrixOffset)
	MENUPROFILES_GET_INHERITED_POINTER_FIELD_VALUE_METHOD(GetMatrixForwardOffset, m_pMatrixOffset, m_flForward)
	MENUPROFILES_GET_INHERITED_POINTER_FIELD_VALUE_METHOD(GetMatrixLeftOffset, m_pMatrixOffset, m_flLeft)
	MENUPROFILES_GET_INHERITED_POINTER_FIELD_VALUE_METHOD(GetMatrixRightOffset, m_pMatrixOffset, m_flRight)
	MENUPROFILES_GET_INHERITED_POINTER_FIELD_VALUE_METHOD(GetMatrixUpOffset, m_pMatrixOffset, m_flUp)

	// Colors.
	MENUPROFILES_GET_INHERITED_POINTER_METHOD(GetInactiveColor, m_pInactiveColor)
	MENUPROFILES_GET_INHERITED_POINTER_METHOD(GetActiveColor, m_pActiveColor)
	MENUPROFILES_GET_INHERITED_POINTER_METHOD(GetBackgroundAwayUnits, m_flBackgroundAwayUnits)

	// Entity key values.
	CEntityKeyValues *GetAllocactedEntityKeyValues(CKeyValues3Context *pAllocator = NULL, bool bIncludeBackground = true) const
	{
		CEntityKeyValues *pResult = new CEntityKeyValues(pAllocator, pAllocator ? EKV_ALLOCATOR_EXTERNAL : EKV_ALLOCATOR_NORMAL);

		Assert(pResult);

		if(m_pData)
		{
			pResult->CopyFrom(m_pData);
		}

		Metadata_t::Bases_t vecBaseline = GetMetadata().GetBaseline();

		FOR_EACH_VEC_BACK(vecBaseline, i)
		{
			CEntityKeyValues *pCopyData = vecBaseline[i]->m_pData;

			if(pCopyData)
			{
				if(bIncludeBackground)
				{
					pResult->CopyFrom(pCopyData, false, false);
				}
				else
				{
					FOR_EACH_ENTITYKEY(pCopyData, it)
					{
						auto aKeyId = pCopyData->GetEntityKeyId(it);

						if(!V_strstr(aKeyId.GetString(), "background"))
						{
							auto *pKVToSet = pResult->SetKeyValue(aKeyId);

							if(pKVToSet)
								*pKVToSet = *pCopyData->GetKeyValue(it);
						}
					}
				}
			}
		}

		return pResult;
	}
};

/**
 * @brief A Menu Profile interface.
**/
class IMenuProfiles
{
public:
	virtual MenuProfile_t *Get(const char *pszName = MENUPROFILES_DEFAULT_NAME) = 0;
	virtual void AddOrReplaceRef(const char *pszName, MenuProfile_t *pData) = 0;

public:
	virtual CKeyValues3Context *GetEntityKeyValuesAllocator() = 0;
}; // IMenuProfile

#endif // _INCLUDE_METAMOD_SOURCE_IMENUPROFILES_HPP_
