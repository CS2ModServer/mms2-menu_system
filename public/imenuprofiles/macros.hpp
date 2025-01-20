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


#ifndef _INCLUDE_METAMOD_SOURCE_IMENUPROFILES_MACROS_HPP_
#	define _INCLUDE_METAMOD_SOURCE_IMENUPROFILES_MACROS_HPP_

#	define MENUPROFILES_GET_INHERITED_VALUE_METHOD(methodName, fieldName) \
	inline const auto methodName() const \
	{ \
		auto aResult = fieldName; \
	\
		if(!aResult) \
		{ \
			for(const auto &pInherited : GetMetadata().GetBaseline()) \
			{ \
				if(aResult = pInherited->fieldName) \
				{ \
					break; \
				} \
			} \
		} \
	\
		return aResult; \
	}
#	define MENUPROFILES_GET_INHERITED_STRING_METHOD(methodName, fieldName) \
	inline auto methodName() \
	{ \
		auto sResult = fieldName; \
	\
		if(sResult.IsEmpty()) \
		{ \
			for(const auto &pInherited : GetMetadata().GetBaseline()) \
			{ \
				if(!(sResult = pInherited->fieldName).IsEmpty()) \
				{ \
					break; \
				} \
			} \
		} \
	\
		return sResult; \
	}
#	define MENUPROFILES_GET_INHERITED_POINTER_METHOD(methodName, fieldName) MENUPROFILES_GET_INHERITED_VALUE_METHOD(methodName, fieldName)
#	define MENUPROFILES_GET_INHERITED_POINTER_FIELD_VALUE_METHOD(methodName, fieldName, subfieldName)\
	inline const auto methodName() const \
	{ \
		auto aResult = fieldName ? fieldName->subfieldName : 0; \
	\
		if(!aResult) \
		{ \
			for(const auto &pInherited : GetMetadata().GetBaseline()) \
			{ \
				if(pInherited->fieldName && (aResult = pInherited->fieldName->subfieldName)) \
				{ \
					break; \
				} \
			} \
		} \
	\
		return aResult; \
	}

#endif // _INCLUDE_METAMOD_SOURCE_IMENUPROFILES_MACROS_HPP_
