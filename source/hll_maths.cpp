// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_maths.h"

Vector HLL::Maths::LHToRHCoordinate(const Vector &v)
{
	return Vector(-v.x, v.y, v.z);
}

Vector HLL::Maths::RHToLHCoordinate(const Vector &v)
{
	return LHToRHCoordinate(v);
}

Vector HLL::Maths::LHToRHRotation(const Vector &hpb)
{
	Matrix LH_H = MatrixRotY(hpb.x);
	Matrix LH_P = MatrixRotX(hpb.y);
	Matrix LH_B = MatrixRotZ(hpb.z);

	Matrix c = LH_H * LH_P * LH_B;
	c.sqmat.v1.y *= -1;
	c.sqmat.v1.z *= -1;
	c.sqmat.v2.x *= -1;
	c.sqmat.v3.x *= -1;

	return MatrixToHPB(c, ROTATIONORDER::HPB);
}

Vector HLL::Maths::RHToLHRotation(const Vector &hpb)
{
	return LHToRHRotation(hpb);
}