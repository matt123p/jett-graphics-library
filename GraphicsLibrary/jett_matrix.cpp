//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "jett.h"
#include <math.h>

/*!
 * \brief Create a matrix object and set all values
 *
 * Create a new matrix and pass in all 6 values for the matrix.  Remember although the matrix is
 * a 3x3 matrix that some of the matrix values are fixed.
 *
 */
jett_matrix::jett_matrix(float _a, float _b, float _c, float _d, float _e, float _f)
{
	a = _a;
	b = _b;
	c = _c;
	d = _d;
	e = _e;
	f = _f;
}

/*!
 * \brief Create a matrix object and the 2x2 values
 *
 * Create a new matrix and pass in all 4 values for the matrix.  The last row of
 * the matrix is left as zero, making this matrix have the same effect as a 2x2 matrix,
 * that is it will not apply a transform.
 *
 */
jett_matrix::jett_matrix(float _a, float _b, float _c, float _d)
{
	a = _a;
	b = _b;
	c = _c;
	d = _d;
	e = 0.0f;
	f = 0.0f;
}

/*!
 * \brief Create a the identity matrix
 *
 * Create a new matrix initialised as the identity matrix.
 *
 */
jett_matrix::jett_matrix()
{
	a = 1.0f;
	b = 0.0f;
	c = 0.0f;
	d = 1.0f;
	e = 0.0f;
	f = 0.0f;
}

/*!
 * \brief Apply this matrix to a single point
 *
 * \param p The point to multiply by the matrix
 *
 * This function multiplies the 2d co-ordinate by the matrix
 * and returns the result.
 *
 */
jett_point jett_matrix::apply(const jett_point& p) const
{
	return jett_point(a * p.x + c * p.y + e, b * p.x + d * p.y + f);
}

/*!
 * \brief Calculate the inverse of this matrix
 *
 * This function calculates the inverse of this matrix and returns
 * it.
 *
 * Note that not all matricies have an inverse.  If the inverse
 * cannot be calculated this function will throw an exception.
 *
 */
jett_matrix jett_matrix::invert() const
{
	jett_matrix r;
	double det = a * d - c * b;

	// A determinant of zero means the matrix
	// cannot be inverted
	if (det == 0)
	{
		throw jett_exception(JETT_MATRIX_CANNOT_INVERT, 0, "The matrix cannot be inverted");
		return r;
	}

	r.a = static_cast<float>(d / det);
	r.b = static_cast<float>(-b / det);
	r.c = static_cast<float>(-c / det);
	r.d = static_cast<float>(a / det);
	r.e = static_cast<float>((c * f - e * d) / det);
	r.f = static_cast<float>(-(a * f - b * e) / det);
	return r;
}

/*!
 * \brief Is this matrix the identity matrix?
 *
 * This function returns true if the matrix is the identity matrix.
 *
 */
bool jett_matrix::is_ident() const
{
	return a == 1.0f && b == 0.0f && c == 0.0f && d == 1.0f && e == 0.0f && f == 0.0f;
}

/*!
 * \brief  Rotate this matrix by the angle
 *
 * \param angle             The angle to rotate by (in radians)
 *
 * The matrix is multiplied by a matrix that has the effect of rotation
 * by the angle.
 *
 * The matrix is then set as this product.
 *
 */
void jett_matrix::rotate(double angle)
{
	// Create a rotation matrix
	jett_matrix rotation(static_cast<float>(cos(angle)), static_cast<float>(sin(angle)), static_cast<float>(-sin(angle)), static_cast<float>(cos(angle)));

	// Now append this matrix to the current one
	append(rotation);
}

/*!
 * \brief  Translate this matrix by an amount
 *
 * \param p             The size of the translation
 *
 * The matrix is multiplied by a matrix that has the effect of a translation
 * by point.
 *
 * The matrix is then set as this product.
 *
 */
void jett_matrix::translate(jett_point p)
{
	// Create a rotation matrix
	jett_matrix translation(1.0f, 0.0f, 0.0f, 1.0f, p.x, p.y);

	// Now append this matrix to the current one
	append(translation);
}

/*!
 * \brief Multiply this matrix by another one
 *
 * \param m             The matrix to multiply by
 *
 * The matrix is multiplied by the matrix.  The matrix is then set as this product.
 *
 */
void jett_matrix::append(const jett_matrix& m)
{
	jett_matrix r(
		a * m.a + b * m.c, a * m.b + b * m.d,
		c * m.a + d * m.c, c * m.b + d * m.d,
		e * m.a + f * m.c + m.e, e * m.b + f * m.d + m.f);

	*this = r;
}