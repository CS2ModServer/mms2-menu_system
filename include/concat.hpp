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
	T m_aHeadWith;
	T m_aStartWith;
	T m_aBefore;
	T m_aBetween;
	T m_aEnd;
	T m_aEndAndStartWith;
}; // ConcatLine_t<T>

template<class T>
class CConcatLineStringImpl : public ConcatLine_t<T>
{
public:
	using Base_t = ConcatLine_t<T>;

protected:
	template<bool INSERT_BEFORE = true>
	inline std::vector<T> GetHeadConcat(const T &aHead) const
	{
		std::vector<T> vecResult {Base_t::m_aHeadWith, aHead};

		if constexpr (INSERT_BEFORE)
		{
			vecResult.push_back(Base_t::m_aBefore);
		}

		vecResult.push_back(Base_t::m_aEnd);

		return vecResult;
	}

	template<bool INSERT_BEFORE = true>
	inline std::vector<T> GetStringHeadConcat(const T &aHead) const
	{
		std::vector<T> vecResult {Base_t::m_aHeadWith, "\"", aHead, "\""};

		if constexpr (INSERT_BEFORE)
		{
			vecResult.push_back(Base_t::m_aBefore);
		}

		vecResult.push_back(Base_t::m_aEnd);

		return vecResult;
	}

	template<bool INSERT_BEFORE = true>
	inline std::vector<T> GetKeyConcat(const T &aKey) const
	{
		std::vector<T> vecResult {Base_t::m_aStartWith, aKey};

		if constexpr (INSERT_BEFORE)
		{
			vecResult.push_back(Base_t::m_aBefore);
		}

		vecResult.push_back(Base_t::m_aEnd);

		return vecResult;
	}

	inline std::vector<T> GetKeyValueConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartWith, aKey, Base_t::m_aBetween, aValue, Base_t::m_aEnd};
	}

	inline std::vector<T> GetKeyValueConcat(const T &aKey, const std::vector<T> &vecValues) const
	{
		std::vector<T> vecResult = {Base_t::m_aStartWith, aKey, Base_t::m_aBefore};

		vecResult.insert(vecResult.cend(), vecValues.cbegin(), vecValues.cend());
		vecResult.push_back(Base_t::m_aEnd);

		return vecResult;
	}

	inline std::vector<T> GetKeyValueStringConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartWith, aKey, Base_t::m_aBetween, "\"", aValue, "\"", Base_t::m_aEnd};
	}

	inline std::vector<T> GetKeyStringValueStringConcat(const T &aKey, const T &aValue) const
	{
		return {Base_t::m_aStartWith, "\"", aKey, "\"", Base_t::m_aBetween, "\"", aValue, "\"", Base_t::m_aEnd};
	}
}; // CConcatLineStringImpl<T>

using CConcatLineStringBaseImpl = CConcatLineStringImpl<const char *>;

class CConcatLineStringBase : public CConcatLineStringBaseImpl
{
public:
	using Impl = CConcatLineStringBaseImpl;

	CConcatLineStringBase() = delete;

public:
	const char *GetHeadWith() const
	{
		return m_aHeadWith;
	}

	const char *GetStartWith() const
	{
		return m_aStartWith;
	}

	const char *GetBefore() const
	{
		return m_aBefore;
	}

	const char *GetBetween() const
	{
		return m_aBetween;
	}

	const char *GetEnd() const
	{
		return m_aEnd;
	}

	const char *GetEndAndStartWith() const
	{
		return m_aEndAndStartWith;
	}
}; // CConcatLineStringBase

class CConcatLineString : public CConcatLineStringBase
{
public:
	using Base = CConcatLineStringBase;
	using Base::GetHeadWith;
	using Base::GetStartWith;
	using Base::GetBefore;
	using Base::GetBetween;
	using Base::GetEnd;
	using Base::GetEndAndStartWith;

public:
	const char *AppendHeadToBuffer(CBufferString &sMessage, const char *pszHeadKey) const;
	const char *AppendStringHeadToBuffer(CBufferString &sMessage, const char *pszHeadKey) const;
	const char *AppendStringHeadWithoutBeforeToBuffer(CBufferString &sMessage, const char *pszHeadKey) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey) const;
	const char *AppendWithoutBeforeToBuffer(CBufferString &sMessage, const char *pszKey) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, bool bValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, int nValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, uint nValue) const;
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

	int AppendToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const;
	int AppendStringToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const;
}; // CConcatLineString

// Globals.

extern const std::array<const CConcatLineString, 8> g_arrEmbedsConcat;

// Backward compatibility.
#define g_aEmbedConcat g_arrEmbedsConcat[0]
#define g_aEmbed2Concat g_arrEmbedsConcat[1] // Next nesting.

#endif // _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_
