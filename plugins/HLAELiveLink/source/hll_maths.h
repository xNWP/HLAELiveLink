// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef HLL_MATHS_H__
#define HLL_MATHS_H__

#include "c4d.h"

namespace HLL
{
	namespace Maths
	{
		//------------------------------------------------------//
		/// Converts a point from left-handed system to right-handed system.
		/// @param[in] v				The vector to convert (in left-handed coordinates).
		/// @return Vector				The converted vector (in right-handed coordinates).
		//------------------------------------------------------//
		Vector LHToRHCoordinate(const Vector &v);

		//------------------------------------------------------//
		/// Converts a point from right-handed system to left-handed system.
		/// @param[in] v				The vector to convert (in right-handed coordinates).
		/// @return Vector				The converted vector (in left-handed coordinates).
		//------------------------------------------------------//
		Vector RHToLHCoordinate(const Vector &v);

		//------------------------------------------------------//
		/// Converts the euler angles HPB from the left-handed to the right-handed system.
		/// @param[in] hpb				The angles to convert (in left-handed angles).
		/// @return Vector				The converted angles (in right-handed angles).
		//------------------------------------------------------//
		Vector LHToRHRotation(const Vector &hpb);

		//------------------------------------------------------//
		/// Converts the euler angles HPB from the right-handed to the left-handed system.
		/// @param[in] hpb				The angles to convert (in right-handed angles).
		/// @return Vector				The converted angles (in left-handed angles).
		//------------------------------------------------------//
		Vector RHToLHRotation(const Vector &hpb);
	}
}

#endif