! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =2      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =6      ! # of nodes for each element
   integer,parameter        :: nndof =nnde*nddof

   integer,parameter        :: nsdim =ndim-1 ! Surface dimension of proplem
   integer,parameter        :: nsrf  =3      ! # of nodes for element surface

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

   integer,parameter :: nint=3

   real(kind=DP_REAL_KIND) dndxl(ndim ,nnde ,nint )
   real(kind=DP_REAL_KIND) cnxl (nnde ,      nint )
!  real(kind=DP_REAL_KIND) cvnn (nnde ,nnde ,nint )
   real(kind=DP_REAL_KIND) xlgp (ndim ,      nint )
   real(kind=DP_REAL_KIND) wint (            nint )

   integer,parameter :: nsint=3
   real(kind=DP_REAL_KIND) dmdxl (      nsrf ,nsint)
   real(kind=DP_REAL_KIND) cmxl  (      nsrf ,nsint)
   real(kind=DP_REAL_KIND) wsint (            nsint)

contains

   subroutine initcf

      implicit none

      real(kind=DP_REAL_KIND),dimension(2) :: xlig2

      integer,dimension(ndim,nint),parameter :: idg2             &
  &      =reshape( (/ 1,1, 2,1, 1,2 /),(/ndim,nint/) )

      real(kind=DP_REAL_KIND),dimension(3) :: xligs3
      real(kind=DP_REAL_KIND),dimension(3) :: wigs3

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl
      real(kind=DP_REAL_KIND)                 :: xll
      integer                 :: intg

      xlig2(1)=d6
      xlig2(2)=d3*2

      do intg=1,nint
         xl(1)=xlig2(idg2(1,intg))
         xl(2)=xlig2(idg2(2,intg))

         xlgp(:,intg)=xl(:)
         wint(intg)=d6

         dndxl(1,1,intg)=-three+four*xl(1)+four*xl(2)
         dndxl(2,1,intg)=-three+four*xl(1)+four*xl(2)
         dndxl(1,2,intg)=four*xl(1)-one
         dndxl(2,2,intg)=zero
         dndxl(1,3,intg)=zero
         dndxl(2,3,intg)=four*xl(2)-one

         dndxl(1,4,intg)=four-eight*xl(1)-four*xl(2)
         dndxl(2,4,intg)=-four*xl(1)
         dndxl(1,5,intg)= four*xl(2)
         dndxl(2,5,intg)= four*xl(1)
         dndxl(1,6,intg)=-four*xl(2)
         dndxl(2,6,intg)=four-four*xl(1)-eight*xl(2)

         cnxl(1,intg)=(one-xl(1)-xl(2))*(one-two*xl(1)-two*xl(2))
         cnxl(2,intg)=xl(1)            *(two*xl(1)-one)
         cnxl(3,intg)=xl(2)            *(two*xl(2)-one)
         cnxl(4,intg)=four*(one-xl(1)-xl(2))*xl(1)
         cnxl(5,intg)=four*xl(1)            *xl(2)
         cnxl(6,intg)=four*xl(2)            *(one-xl(1)-xl(2))

      end do

!     Surface element

      xligs3(3)= sqrt(3.0d0/5.0d0)
      xligs3(2)=zero
      xligs3(1)=-xligs3(3)

      wigs3(1)=5.0d0/9.0d0
      wigs3(2)=8.0d0/9.0d0
      wigs3(3)=wigs3(1)

      do intg=1,nsint

         xll=xligs3(intg)
         wsint(intg)=wigs3(intg)

         dmdxl(1,intg)=xll-d2
         dmdxl(2,intg)=xll+d2
         dmdxl(3,intg)=-two*xll

         cmxl(1,intg)=-xll*(one-xll)*d2
         cmxl(2,intg)= xll*(one+xll)*d2
         cmxl(3,intg)=(one+xll)*(one-xll)
      end do

   end subroutine initcf

end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
