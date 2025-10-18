! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module kind_parameters

!  The default kinds

   implicit none
   integer, parameter :: DEFAULT_INTEGER_KIND = kind (0)
   integer, parameter :: DEFAULT_LOGICAL_KIND = kind (.false.)
   integer, parameter :: DEFAULT_REAL_KIND = kind (0.0)
   integer, parameter :: DP_REAL_KIND = kind (0.0d0)
   integer, parameter :: DEFAULT_CHARACTER_KIND = kind ("A")

end module kind_parameters

module dconst

   use kind_parameters

   implicit none
   real(kind=DP_REAL_KIND), parameter :: zero  =0.d0
   real(kind=DP_REAL_KIND), parameter :: one   =1.d0
   real(kind=DP_REAL_KIND), parameter :: two   =2.d0
   real(kind=DP_REAL_KIND), parameter :: three =3.d0
   real(kind=DP_REAL_KIND), parameter :: four  =4.d0
   real(kind=DP_REAL_KIND), parameter :: eight =8.d0
   real(kind=DP_REAL_KIND), parameter :: d2    =0.5d0
   real(kind=DP_REAL_KIND), parameter :: d3    =0.333333333333333333333d0
   real(kind=DP_REAL_KIND), parameter :: d4    =0.25d0
   real(kind=DP_REAL_KIND), parameter :: d6    =0.166666666666666666667d0
   real(kind=DP_REAL_KIND), parameter :: d8    =0.125d0
   real(kind=DP_REAL_KIND), parameter :: d12   =0.0833333333333333333333d0
   real(kind=DP_REAL_KIND), parameter :: d24   =0.0416666666666666666667d0 
   real(kind=DP_REAL_KIND), parameter :: d36   =0.0277777777777777777778d0
   real(kind=DP_REAL_KIND), parameter :: d20   =0.05d0
   real(kind=DP_REAL_KIND), parameter :: d60   =0.0166666666666666666667d0
   real(kind=DP_REAL_KIND), parameter :: d120  =0.0083333333333333333333d0

   real(kind=DP_REAL_KIND), parameter :: fd3   =1.333333333333333333333d0
   real(kind=DP_REAL_KIND), parameter :: td3   =0.666666666666666666667d0 

   real(kind=DP_REAL_KIND), parameter :: pi    =3.14159265358979d0

end module dconst


! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
