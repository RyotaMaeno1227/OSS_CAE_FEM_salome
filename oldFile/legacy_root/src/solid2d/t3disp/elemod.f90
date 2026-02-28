! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =2      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =3      ! # of nodes for each element

   integer,parameter        :: nsdim =ndim-1 ! Surface dimension of proplem
   integer,parameter        :: nsrf  =2      ! # of nodes for element surface

   integer,parameter        :: nndof =nnde*nddof
   integer,parameter        :: nstr  =3      ! Number of Strain Component

   integer,parameter        :: mtype =2      ! Material type (plane stress)
   integer,parameter        :: nstrss=3
!   integer,parameter        :: mtype =1      ! Material type (plane strain)
!   integer,parameter        :: nstrss=4

end module mddim

module cfmc

   use dconst
   use mddim

   implicit none

   integer,parameter :: nint=1

   real(kind=DP_REAL_KIND) dndxl(ndim ,nnde ,nint )
   real(kind=DP_REAL_KIND) cnxl (nnde ,      nint )
!  real(kind=DP_REAL_KIND) cvnn (nnde ,nnde ,nint )
   real(kind=DP_REAL_KIND) wint (            nint )

contains

   subroutine initcf

      implicit none

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl
      integer                 :: intg

      intg=1
      xl(1)=d3
      xl(2)=d3

      wint(intg)=d2

      dndxl(1,1,intg)=-one
      dndxl(2,1,intg)=-one
      dndxl(1,2,intg)= one
      dndxl(2,2,intg)= zero
      dndxl(1,3,intg)= zero
      dndxl(2,3,intg)= one

      cnxl(1,intg)=one-xl(1)-xl(2)
      cnxl(2,intg)=xl(1)
      cnxl(3,intg)=xl(2)

   end subroutine initcf

end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
