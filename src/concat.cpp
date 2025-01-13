
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

#include <concat.hpp>

const CConcatLineString g_aEmbedConcat 
{
	"",     // Head with.
	"\t",   // Start with.
	":",    // Before value.
	": ",   // Between key & value.
	"\n",   // End.
	"\n\t", // End and next line.
};

const CConcatLineString g_aEmbed2Concat 
{
	"\t",
	"\t\t",
	":",
	": ",
	"\n",
	"\n\t\t",
};

const CConcatLineString g_aEmbed3Concat 
{
	"\t\t",
	"\t\t\t",
	":",
	": ",
	"\n",
	"\n\t\t\t",
};

const CConcatLineString g_aEmbed4Concat 
{
	"\t\t\t",
	"\t\t\t\t",
	":",
	": ",
	"\n",
	"\n\t\t\t\t",
};

const CConcatLineString g_aEmbed5Concat 
{
	"\t\t\t\t",
	"\t\t\t\t\t",
	":",
	": ",
	"\n",
	"\n\t\t\t\t\t",
};

const char *CConcatLineString::AppendHeadToBuffer(CBufferString &sMessage, const char *pszHeadKey) const
{
	const auto vecConcat = Base::GetHeadConcat(pszHeadKey);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *CConcatLineString::AppendStringHeadToBuffer(CBufferString &sMessage, const char *pszHeadKey) const
{
	const auto vecConcat = Base::GetStringHeadConcat(pszHeadKey);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey) const
{
	const auto vecConcat = Base::GetKeyConcat(pszKey);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, bool bValue) const
{
	return AppendToBuffer(sMessage, pszKey, bValue ? "true" : "false");
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, int nValue) const
{
	char sValue[12];

	V_snprintf(sValue, sizeof(sValue), "%i", nValue);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, uint nValue) const
{
	char sValue[11];

	V_snprintf(sValue, sizeof(sValue), "%u", nValue);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, float flValue) const
{
	char sValue[24];

	V_snprintf(sValue, sizeof(sValue), "%f", flValue);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, const Vector &vecValue) const
{
	char sValue[72];

	V_snprintf(sValue, sizeof(sValue), "%.6f %.6f %.6f", vecValue.x, vecValue.y, vecValue.z);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, const QAngle &angValue) const
{
	char sValue[72];

	V_snprintf(sValue, sizeof(sValue), "%.6f %.6f %.6f", angValue.x, angValue.y, angValue.z);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyValueConcat(pszKey, pszValue);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *CConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, std::vector<const char *> vecValues) const
{
	const auto vecConcat = GetKeyValueConcat(pszKey, vecValues);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *CConcatLineString::AppendBytesToBuffer(CBufferString &sMessage, const char *pszKey, const byte *pData, uintp nLength) const
{
	std::vector<const char *> vecValues;

	vecValues.reserve(nLength);

	CBufferStringGrowable<4> *psDataSet = new CBufferStringGrowable<4>[nLength];

	for(int n = 0; n < nLength; n++)
	{
		auto &sByte = psDataSet[n];

		sByte.Format("%02X ", pData[n]);
		vecValues.push_back(sByte.Get());
	}

	auto *pResult = AppendToBuffer(sMessage, pszKey, vecValues);

	delete[] psDataSet;

	return pResult;
}

const char *CConcatLineString::AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint32 uHandle) const
{
	char sHandle[11];

	V_snprintf(sHandle, sizeof(sHandle), "%u", uHandle);

	return AppendToBuffer(sMessage, pszKey, sHandle);
}

const char *CConcatLineString::AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint64 uHandle) const
{
	char sHandle[21];

	V_snprintf(sHandle, sizeof(sHandle), "%llu", uHandle);

	return AppendToBuffer(sMessage, pszKey, sHandle);
}

const char *CConcatLineString::AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, const void *pHandle) const
{
	return AppendHandleToBuffer(sMessage, pszKey, (uint64)pHandle);
}

const char *CConcatLineString::AppendPointerToBuffer(CBufferString &sMessage, const char *pszKey, const void *pValue) const
{
	char sPointer[19];

	V_snprintf(sPointer, sizeof(sPointer), "%p", pValue);

	return AppendToBuffer(sMessage, pszKey, sPointer);
}

const char *CConcatLineString::AppendStringToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyStringConcat(pszKey, pszValue);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

int CConcatLineString::AppendToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyValueConcat(pszKey, pszValue);

	return vecMessage.AddMultipleToTail(vecConcat.size(), vecConcat.data());
}

int CConcatLineString::AppendStringToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyStringConcat(pszKey, pszValue);

	return vecMessage.AddMultipleToTail(vecConcat.size(), vecConcat.data());
}
