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

#ifndef _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_
#	define _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_

#	pragma once

#	include <stddef.h>

#	include <array>
#	include <vector>

#	include <tier0/platform.h>
#	include <tier0/bufferstring.h>
#	include <tier1/utlvector.h>
#	include <mathlib/vector.h>

template<class T>
struct ConcatLine_t
{
	ConcatLine_t(const T &aHeadsWith, const T &aStartsWith, const T &aBefore, const T &aBetween, const T &aEnds, const T &aEndsAndStartsWith)
	 :  m_aHeadsWith(aHeadsWith), 
	    m_aStartsWith(aStartsWith), 
	    m_aBefore(aBefore), 
	    m_aBetween(aBetween), 
	    m_aEnds(aEnds), 
	    m_aEndsAndStartsWith(aEndsAndStartsWith)
	{
	}

	T m_aHeadsWith;
	T m_aStartsWith;
	T m_aBefore;
	T m_aBetween;
	T m_aEnds;
	T m_aEndsAndStartsWith;
}; // ConcatLine_t<T>

template<class T>
class CConcatLineStringImpl : public ConcatLine_t<T>
{
public:
	using Base_t = ConcatLine_t<T>;
	using Base_t::Base_t;
	CConcatLineStringImpl(const Base_t &&aBase)
	 :  Base_t(aBase)
	{
	}

protected:
	template<bool INSERT_BEFORE = true>
	inline auto GetHeadConcat(const T &aHead) const
	{
		std::array<T, 3 + INSERT_BEFORE> arrResult {Base_t::m_aHeadsWith, aHead};

		if constexpr (INSERT_BEFORE)
		{
			arrResult[2] = Base_t::m_aBefore;
		}

		arrResult[2 + INSERT_BEFORE] = Base_t::m_aEnds;

		return arrResult;
	}

	template<bool INSERT_BEFORE = true>
	inline auto GetStringHeadConcat(const T &aHead) const
	{
		std::array<T, 5 + INSERT_BEFORE> arrResult {Base_t::m_aHeadsWith, "\"", aHead, "\""};

		if constexpr (INSERT_BEFORE)
		{
			arrResult[4] = Base_t::m_aBefore;
		}

		arrResult[4 + INSERT_BEFORE] = Base_t::m_aEnds;

		return arrResult;
	}

	template<bool INSERT_BEFORE = true>
	inline auto GetKeyConcat(const T &aKey) const
	{
		std::array<T, 3 + INSERT_BEFORE> arrResult {Base_t::m_aStartsWith, aKey};

		if constexpr (INSERT_BEFORE)
		{
			arrResult[2] = Base_t::m_aBefore;
		}

		arrResult[2 + INSERT_BEFORE] = Base_t::m_aEnds;

		return arrResult;
	}

	inline std::array<T, 5> GetKeyValueConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartsWith, aKey, Base_t::m_aBetween, aValue, Base_t::m_aEnds};
	}

	inline auto GetKeyValueConcat(const T &aKey, const std::vector<T> &vecValues) const
	{
		std::vector<T> vecResult = {Base_t::m_aStartsWith, aKey, Base_t::m_aBetween};

		vecResult.insert(vecResult.cend(), vecValues.cbegin(), vecValues.cend());
		vecResult.push_back(Base_t::m_aEnds);

		return vecResult;
	}

	inline std::array<T, 7> GetKeyStringValueConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartsWith, "\"", aKey, "\"", Base_t::m_aBetween, aValue, Base_t::m_aEnds};
	}

	inline std::array<T, 7> GetKeyValueStringConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartsWith, aKey, Base_t::m_aBetween, "\"", aValue, "\"", Base_t::m_aEnds};
	}

	inline std::array<T, 9> GetKeyStringValueStringConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartsWith, "\"", aKey, "\"", Base_t::m_aBetween, "\"", aValue, "\"", Base_t::m_aEnds};
	}
}; // CConcatLineStringImpl<T>

using CConcatLineStringBaseImpl = CConcatLineStringImpl<const char *>;

class CConcatLineStringBase : public CConcatLineStringBaseImpl
{
public:
	using Impl = CConcatLineStringBaseImpl;
	using Impl::Impl;

public:
	const char *GetHeadsWith() const
	{
		return m_aHeadsWith;
	}

	const char *GetStartsWith() const
	{
		return m_aStartsWith;
	}

	const char *GetBefore() const
	{
		return m_aBefore;
	}

	const char *GetBetween() const
	{
		return m_aBetween;
	}

	const char *GetEnds() const
	{
		return m_aEnds;
	}

	const char *GetEndsAndStartsWith() const
	{
		return m_aEndsAndStartsWith;
	}
}; // CConcatLineStringBase

class CConcatLineString : public CConcatLineStringBase
{
public:
	using Base = CConcatLineStringBase;
	using Base::Base;
	using Base::GetHeadsWith;
	using Base::GetStartsWith;
	using Base::GetBefore;
	using Base::GetBetween;
	using Base::GetEnds;
	using Base::GetEndsAndStartsWith;

public:
	const char *AppendHeadToBuffer(CBufferString &sMessage, const char *pszHeadKey) const;
	const char *AppendStringHeadToBuffer(CBufferString &sMessage, const char *pszHeadKey) const;
	const char *AppendStringHeadWithoutBeforeToBuffer(CBufferString &sMessage, const char *pszHeadKey) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey) const;
	const char *AppendWithoutBeforeToBuffer(CBufferString &sMessage, const char *pszKey) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, bool bValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, int nValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, uint nValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, uint64 nValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, float flValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, const Vector &vecValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, const QAngle &angValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, double dblValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, std::vector<const char *> vecValues) const;
	const char *AppendBytesToBuffer(CBufferString &sMessage, const char *pszKey, const byte *pData, uintp nLength) const;
	const char *AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint32 uHandle) const;
	const char *AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint64 uHandle) const;
	const char *AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, const void *pHandle) const;
	const char *AppendPointerToBuffer(CBufferString &sMessage, const char *pszKey, const void *pValue) const;
	const char *AppendStringToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const;
	const char *AppendKeyStringValueStringToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const;
	const char *AppendKeyStringValuePointerToBuffer(CBufferString &sMessage, const char *pszKey, const void *pValue) const;
	const char *AppendEndsToBuffer(CBufferString &sMessage) const;
	const char *AppendEndsAndStartsToBuffer(CBufferString &sMessage) const;

	int AppendToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const;
	int AppendStringToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const;
}; // CConcatLineString

// Globals.
extern const std::array<const CConcatLineString, 8> g_arrEmbedsConcat;

// Bcompatibility.
#define g_aEmbedConcat g_arrEmbedsConcat[0]
#define g_aEmbed2Concat g_arrEmbedsConcat[1] // Next nesting.
#define g_aEmbed3Concat g_arrEmbedsConcat[2] // Next nesting after the double.

// Menu thing.
extern const CConcatLineString g_aMenuConcat;

#endif // _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_
