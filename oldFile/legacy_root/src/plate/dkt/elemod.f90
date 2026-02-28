! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =2      ! Space dimension of proplem
   integer,parameter        :: nddof =3      ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =3      ! # of nodes for each element
   integer,parameter        :: nndof =nnde*nddof

   integer,parameter        :: nsdim =ndim   ! Surface dimension of proplem
   integer,parameter        :: nsrf  =2      ! # of nodes for element surface

   integer,parameter        :: nstr  =3      ! Number of Strain Component

   integer,parameter        :: mtype =5      ! Material type
   integer,parameter        :: nstrss=3      ! Number of Stress Component

end module mddim

module cfmc

   use dconst
   use mddim

   implicit none

   integer,parameter :: nint=1

   real(kind=DP_REAL_KIND) dndxl(ndim ,nnde ,nint )
!  real(kind=DP_REAL_KIND) cnxl (nnde ,      nint )
!  real(kind=DP_REAL_KIND) cvnn (nnde ,nnde ,nint )
!  real(kind=DP_REAL_KIND) xlgp (ndim       ,nint )
   real(kind=DP_REAL_KIND) wint (            nint )

contains

   subroutine initcf

      implicit none

      integer                 :: intg

      intg=1
!     xlgp(1)=d3
!     xlgp(2)=d3

      wint(intg)=d2

      dndxl(1,1,intg)=-one
      dndxl(2,1,intg)=-one
      dndxl(1,2,intg)= one
      dndxl(2,2,intg)= zero
      dndxl(1,3,intg)= zero
      dndxl(2,3,intg)= one

   end subroutine initcf

end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
