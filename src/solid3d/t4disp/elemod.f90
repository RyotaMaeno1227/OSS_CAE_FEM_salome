! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =3      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =4      ! # of nodes for each element

   integer,parameter        :: nsdim =ndim-1 ! Surface dimension of proplem
   integer,parameter        :: nsrf  =3      ! # of nodes for element surface

   integer,parameter        :: nndof =nnde*nddof
   integer,parameter        :: nstr  =6      ! Number of Strain Component

   integer,parameter        :: mtype =3      ! Material type
   integer,parameter        :: nstrss=6      ! Number of Stress Component

   character(len=3),parameter :: letyp ='tet' ! Element type name

end module mddim

module cfmc

   use dconst
   use mddim

   implicit none

   integer,parameter :: nint =1

   real(kind=DP_REAL_KIND) dndxl (ndim ,nnde ,nint )
   real(kind=DP_REAL_KIND) cnxl  (nnde ,      nint )
!  real(kind=DP_REAL_KIND) cvnn  (nnde ,nnde ,nint )
   real(kind=DP_REAL_KIND) wint  (            nint )
   real(kind=DP_REAL_KIND) xlgp  (ndim ,      nint )

   integer,parameter :: nsint=1
   real(kind=DP_REAL_KIND) dmdxl (nsdim,nsrf ,nsint)
   real(kind=DP_REAL_KIND) cmxl  (      nsrf ,nsint)
   real(kind=DP_REAL_KIND) wsint (            nsint)

contains

   subroutine initcf

      implicit none

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl

      xlgp(:,1)=d4

      wint(1)=d6

      dndxl(1,1,1)=-one
      dndxl(2,1,1)=dndxl(1,1,1)
      dndxl(3,1,1)=dndxl(1,1,1)
      dndxl(1,2,1)=one
      dndxl(2,2,1)=zero
      dndxl(3,2,1)=zero
      dndxl(1,3,1)=zero
      dndxl(2,3,1)=one
      dndxl(3,3,1)=zero
      dndxl(1,4,1)=zero
      dndxl(2,4,1)=zero
      dndxl(3,4,1)=one

      cnxl (  1,1)=d4
      cnxl (  2,1)=d4
      cnxl (  3,1)=d4
      cnxl (  4,1)=d4

!     Surface element

      xl(1)=d3
      xl(2)=d3

      wsint(1)=d2

      dmdxl(1,1,1)=-one
      dmdxl(2,1,1)=-one
      dmdxl(1,2,1)=one
      dmdxl(2,2,1)=zero
      dmdxl(1,3,1)=zero
      dmdxl(2,3,1)=one

      cmxl(1,1)=d3
      cmxl(2,1)=d3
      cmxl(3,1)=d3

   end subroutine initcf
end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
