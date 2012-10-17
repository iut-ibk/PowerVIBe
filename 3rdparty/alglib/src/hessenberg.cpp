/*************************************************************************
Copyright (c) 1992-2007 The University of Tennessee. All rights reserved.

Contributors:
    * Sergey Bochkanov (ALGLIB project). Translation from FORTRAN to
      pseudocode.

See subroutines comments for additional copyrights.

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

#ifdef _WIN32
#include <stdafx.h>
#endif
#include "hessenberg.h"

/*************************************************************************
Reduction of a square matrix to  upper Hessenberg form: Q'*A*Q = H,
where Q is an orthogonal matrix, H - Hessenberg matrix.

Input parameters:
    A       -   matrix A with elements [0..N-1, 0..N-1]
    N       -   size of matrix A.

Output parameters:
    A       -   matrices Q and P in  compact form (see below).
    Tau     -   array of scalar factors which are used to form matrix Q.
                Array whose index ranges within [0..N-2]

Matrix H is located on the main diagonal, on the lower secondary  diagonal
and above the main diagonal of matrix A. The elements which are used to
form matrix Q are situated in array Tau and below the lower secondary
diagonal of matrix A as follows:

Matrix Q is represented as a product of elementary reflections

Q = H(0)*H(2)*...*H(n-2),

where each H(i) is given by

H(i) = 1 - tau * v * (v^T)

where tau is a scalar stored in Tau[I]; v - is a real vector,
so that v(0:i) = 0, v(i+1) = 1, v(i+2:n-1) stored in A(i+2:n-1,i).

  -- LAPACK routine (version 3.0) --
     Univ. of Tennessee, Univ. of California Berkeley, NAG Ltd.,
     Courant Institute, Argonne National Lab, and Rice University
     October 31, 1992
*************************************************************************/
void rmatrixhessenberg(ap::real_2d_array& a, int n, ap::real_1d_array& tau)
{
    int i;
    double v;
    ap::real_1d_array t;
    ap::real_1d_array work;

    ap::ap_error::make_assertion(n>=0, "RMatrixHessenberg: incorrect N!");
    
    //
    // Quick return if possible
    //
    if( n<=1 )
    {
        return;
    }
    tau.setbounds(0, n-2);
    t.setbounds(1, n);
    work.setbounds(0, n-1);
    for(i = 0; i <= n-2; i++)
    {
        
        //
        // Compute elementary reflector H(i) to annihilate A(i+2:ihi,i)
        //
        ap::vmove(t.getvector(1, n-i-1), a.getcolumn(i, i+1, n-1));
        generatereflection(t, n-i-1, v);
        ap::vmove(a.getcolumn(i, i+1, n-1), t.getvector(1, n-i-1));
        tau(i) = v;
        t(1) = 1;
        
        //
        // Apply H(i) to A(1:ihi,i+1:ihi) from the right
        //
        applyreflectionfromtheright(a, v, t, 0, n-1, i+1, n-1, work);
        
        //
        // Apply H(i) to A(i+1:ihi,i+1:n) from the left
        //
        applyreflectionfromtheleft(a, v, t, i+1, n-1, i+1, n-1, work);
    }
}


/*************************************************************************
Unpacking matrix Q which reduces matrix A to upper Hessenberg form

Input parameters:
    A   -   output of RMatrixHessenberg subroutine.
    N   -   size of matrix A.
    Tau -   scalar factors which are used to form Q.
            Output of RMatrixHessenberg subroutine.

Output parameters:
    Q   -   matrix Q.
            Array whose indexes range within [0..N-1, 0..N-1].

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
void rmatrixhessenbergunpackq(const ap::real_2d_array& a,
     int n,
     const ap::real_1d_array& tau,
     ap::real_2d_array& q)
{
    int i;
    int j;
    ap::real_1d_array v;
    ap::real_1d_array work;

    if( n==0 )
    {
        return;
    }
    
    //
    // init
    //
    q.setbounds(0, n-1, 0, n-1);
    v.setbounds(0, n-1);
    work.setbounds(0, n-1);
    for(i = 0; i <= n-1; i++)
    {
        for(j = 0; j <= n-1; j++)
        {
            if( i==j )
            {
                q(i,j) = 1;
            }
            else
            {
                q(i,j) = 0;
            }
        }
    }
    
    //
    // unpack Q
    //
    for(i = 0; i <= n-2; i++)
    {
        
        //
        // Apply H(i)
        //
        ap::vmove(v.getvector(1, n-i-1), a.getcolumn(i, i+1, n-1));
        v(1) = 1;
        applyreflectionfromtheright(q, tau(i), v, 0, n-1, i+1, n-1, work);
    }
}


/*************************************************************************
Unpacking matrix H (the result of matrix A reduction to upper Hessenberg form)

Input parameters:
    A   -   output of RMatrixHessenberg subroutine.
    N   -   size of matrix A.

Output parameters:
    H   -   matrix H. Array whose indexes range within [0..N-1, 0..N-1].

  -- ALGLIB --
     Copyright 2005 by Bochkanov Sergey
*************************************************************************/
void rmatrixhessenbergunpackh(const ap::real_2d_array& a,
     int n,
     ap::real_2d_array& h)
{
    int i;
    int j;
    ap::real_1d_array v;
    ap::real_1d_array work;

    if( n==0 )
    {
        return;
    }
    h.setbounds(0, n-1, 0, n-1);
    for(i = 0; i <= n-1; i++)
    {
        for(j = 0; j <= i-2; j++)
        {
            h(i,j) = 0;
        }
        j = ap::maxint(0, i-1);
        ap::vmove(&h(i, j), &a(i, j), ap::vlen(j,n-1));
    }
}


void toupperhessenberg(ap::real_2d_array& a, int n, ap::real_1d_array& tau)
{
    int i;
    int ip1;
    int nmi;
    double v;
    ap::real_1d_array t;
    ap::real_1d_array work;

    ap::ap_error::make_assertion(n>=0, "ToUpperHessenberg: incorrect N!");
    
    //
    // Quick return if possible
    //
    if( n<=1 )
    {
        return;
    }
    tau.setbounds(1, n-1);
    t.setbounds(1, n);
    work.setbounds(1, n);
    for(i = 1; i <= n-1; i++)
    {
        
        //
        // Compute elementary reflector H(i) to annihilate A(i+2:ihi,i)
        //
        ip1 = i+1;
        nmi = n-i;
        ap::vmove(t.getvector(1, nmi), a.getcolumn(i, ip1, n));
        generatereflection(t, nmi, v);
        ap::vmove(a.getcolumn(i, ip1, n), t.getvector(1, nmi));
        tau(i) = v;
        t(1) = 1;
        
        //
        // Apply H(i) to A(1:ihi,i+1:ihi) from the right
        //
        applyreflectionfromtheright(a, v, t, 1, n, i+1, n, work);
        
        //
        // Apply H(i) to A(i+1:ihi,i+1:n) from the left
        //
        applyreflectionfromtheleft(a, v, t, i+1, n, i+1, n, work);
    }
}


void unpackqfromupperhessenberg(const ap::real_2d_array& a,
     int n,
     const ap::real_1d_array& tau,
     ap::real_2d_array& q)
{
    int i;
    int j;
    ap::real_1d_array v;
    ap::real_1d_array work;
    int ip1;
    int nmi;

    if( n==0 )
    {
        return;
    }
    
    //
    // init
    //
    q.setbounds(1, n, 1, n);
    v.setbounds(1, n);
    work.setbounds(1, n);
    for(i = 1; i <= n; i++)
    {
        for(j = 1; j <= n; j++)
        {
            if( i==j )
            {
                q(i,j) = 1;
            }
            else
            {
                q(i,j) = 0;
            }
        }
    }
    
    //
    // unpack Q
    //
    for(i = 1; i <= n-1; i++)
    {
        
        //
        // Apply H(i)
        //
        ip1 = i+1;
        nmi = n-i;
        ap::vmove(v.getvector(1, nmi), a.getcolumn(i, ip1, n));
        v(1) = 1;
        applyreflectionfromtheright(q, tau(i), v, 1, n, i+1, n, work);
    }
}


void unpackhfromupperhessenberg(const ap::real_2d_array& a,
     int n,
     const ap::real_1d_array& tau,
     ap::real_2d_array& h)
{
    int i;
    int j;
    ap::real_1d_array v;
    ap::real_1d_array work;

    if( n==0 )
    {
        return;
    }
    h.setbounds(1, n, 1, n);
    for(i = 1; i <= n; i++)
    {
        for(j = 1; j <= i-2; j++)
        {
            h(i,j) = 0;
        }
        j = ap::maxint(1, i-1);
        ap::vmove(&h(i, j), &a(i, j), ap::vlen(j,n));
    }
}




