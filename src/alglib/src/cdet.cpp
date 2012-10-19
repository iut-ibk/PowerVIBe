/*************************************************************************
Copyright (c) 2005-2007, Sergey Bochkanov (ALGLIB project).

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 2 of the 
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses

>>> END OF LICENSE >>>
*************************************************************************/

#include <stdafx.h>
#include "cdet.h"

/*************************************************************************
Determinant calculation of the matrix given by its LU decomposition.

Input parameters:
    A       -   LU decomposition of the matrix (output of
                RMatrixLU subroutine).
    Pivots  -   table of permutations which were made during
                the LU decomposition.
                Output of RMatrixLU subroutine.
    N       -   size of matrix A.

Result: matrix determinant.

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
ap::complex cmatrixludet(const ap::complex_2d_array& a,
     const ap::integer_1d_array& pivots,
     int n)
{
    ap::complex result;
    int i;
    int s;

    result = 1;
    s = 1;
    for(i = 0; i <= n-1; i++)
    {
        result = result*a(i,i);
        if( pivots(i)!=i )
        {
            s = -s;
        }
    }
    result = result*s;
    return result;
}


/*************************************************************************
Calculation of the determinant of a general matrix

Input parameters:
    A       -   matrix, array[0..N-1, 0..N-1]
    N       -   size of matrix A.

Result: determinant of matrix A.

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
ap::complex cmatrixdet(ap::complex_2d_array a, int n)
{
    ap::complex result;
    ap::integer_1d_array pivots;

    cmatrixlu(a, n, n, pivots);
    result = cmatrixludet(a, pivots, n);
    return result;
}


ap::complex complexdeterminantlu(const ap::complex_2d_array& a,
     const ap::integer_1d_array& pivots,
     int n)
{
    ap::complex result;
    int i;
    int s;

    result = 1;
    s = 1;
    for(i = 1; i <= n; i++)
    {
        result = result*a(i,i);
        if( pivots(i)!=i )
        {
            s = -s;
        }
    }
    result = result*s;
    return result;
}


ap::complex complexdeterminant(ap::complex_2d_array a, int n)
{
    ap::complex result;
    ap::integer_1d_array pivots;

    complexludecomposition(a, n, n, pivots);
    result = complexdeterminantlu(a, pivots, n);
    return result;
}




