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

#ifndef _INCLUDE_METAMOD_SOURCE_MATH_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MATH_HPP_

#	pragma once

#	include <tier0/platform.h>
#	include <mathlib/vector.h>

const float g_flUnitRadians = 180.f / M_PI_F;

FORCEINLINE Vector AddToFrontByRotation(const Vector &vecOrigin, const QAngle &angRotation, float flUnits)
{
	float flSine, flCosine;

	SinCos(angRotation.y / g_flUnitRadians, &flSine, &flCosine);

	return
	{
		vecOrigin.x + flCosine * flUnits,
		vecOrigin.y + flSine * flUnits,
		vecOrigin.z + -TableSin(angRotation.x / g_flUnitRadians) * flUnits
	};
}

#endif //_INCLUDE_METAMOD_SOURCE_MATH_HPP_
