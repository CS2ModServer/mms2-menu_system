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

template<class T = Vector>
FORCEINLINE T GetDirectionFromAngle(const QAngle &angRotation)
{
	const float flPitchRadians = angRotation.x / g_flUnitRadians;
	const float flYawRadians = angRotation.y / g_flUnitRadians;

	float flPitchSine, flPitchCosine, 
	      flYawSine, flYawCosine;

	SinCos(flPitchRadians, &flPitchSine, &flPitchCosine);
	SinCos(flYawRadians, &flYawSine, &flYawCosine);

	return {flPitchCosine * flYawCosine, flPitchCosine * flYawSine, -flPitchSine};
}

FORCEINLINE Vector AddToFrontByRotation(const Vector &vecOrigin, const QAngle &angRotation, float flDistance)
{
	return vecOrigin + (GetDirectionFromAngle<>(angRotation) * flDistance);
}

#endif //_INCLUDE_METAMOD_SOURCE_MATH_HPP_
