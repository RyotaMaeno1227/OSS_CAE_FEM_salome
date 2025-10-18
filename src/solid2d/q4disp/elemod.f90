! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =2      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =4      ! # of nodes for each element
   integer,parameter        :: nndof =nnde*nddof

   integer,parameter        :: nsdim =ndim-1 ! Surface dimension of proplem
   integer,parameter        :: nsrf  =2      ! # of nodes for element surface

   integer,parameter        :: nstr  =3      ! Number of Strain Component

!   integer,parameter        :: mtype =2      ! Material type (plane stress)
!   integer,parameter        :: nstrss=3
   integer,parameter        :: mtype =1      ! Material type (plane strain)
   integer,parameter        :: nstrss=4

end module mddim

module cfmc

   use dconst
   use mddim

   implicit none

   integer,parameter :: nint=4

   real(kind=DP_REAL_KIND) dndxl(ndim ,nnde ,nint )
   real(kind=DP_REAL_KIND) cnxl (nnde ,      nint )
!  real(kind=DP_REAL_KIND) cvnn (nnde ,nnde ,nint )
   real(kind=DP_REAL_KIND) xlgp (ndim       ,nint )
   real(kind=DP_REAL_KIND) wint (            nint )

contains

   subroutine initcf

      implicit none

      real(kind=DP_REAL_KIND),dimension(2) :: xlig2

      integer,dimension(ndim,nint),parameter :: idg2             &
  &      =reshape( (/ 1,1, 2,1, 2,2, 1,2 /),(/ndim,nint/) )

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl
      integer                 :: intg

      xlig2(1)=-1.0d0/sqrt(3.0d0)
      xlig2(2)= 1.0d0/sqrt(3.0d0)

      do intg=1,nint
         xl(1)=xlig2(idg2(1,intg))
         xl(2)=xlig2(idg2(2,intg))
         xlgp(:,intg)=xl(:)

         wint(intg)=one

         dndxl(1,1,intg)=-one       *(one-xl(2))*d4
         dndxl(2,1,intg)=(one-xl(1))*(-one)     *d4
         dndxl(1,2,intg)= one       *(one-xl(2))*d4
         dndxl(2,2,intg)=(one+xl(1))*(-one)     *d4
         dndxl(1,3,intg)= one       *(one+xl(2))*d4
         dndxl(2,3,intg)=(one+xl(1))* one       *d4
         dndxl(1,4,intg)=-one       *(one+xl(2))*d4
         dndxl(2,4,intg)=(one-xl(1))* one       *d4

         cnxl(1,intg)=(one-xl(1))*(one-xl(2))*d4
         cnxl(2,intg)=(one+xl(1))*(one-xl(2))*d4
         cnxl(3,intg)=(one+xl(1))*(one+xl(2))*d4
         cnxl(4,intg)=(one-xl(1))*(one+xl(2))*d4

      end do

   end subroutine initcf

end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
