! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =2      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =9      ! # of nodes for each element
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

   integer,parameter :: nint=9

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

      integer,dimension(ndim,nint),parameter :: idg2             &
  &    =reshape( (/ 1,1, 3,1, 3,3, 1,3, 2,1, 3,2, 2,3, 1,2, 2,2 /),&
  &              (/ndim,nint/) )

      real(kind=DP_REAL_KIND),dimension(3) :: xligs3
      real(kind=DP_REAL_KIND),dimension(3) :: wigs3

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl
      real(kind=DP_REAL_KIND)                 :: xll
      integer                 :: intg

!     3 point Gauss rule

      xligs3(3)= sqrt(3.0d0/5.0d0)
      xligs3(2)=zero
      xligs3(1)=-xligs3(3)

      wigs3(1)=5.0d0/9.0d0
      wigs3(2)=8.0d0/9.0d0
      wigs3(3)=wigs3(1)

!     Shape function

      do intg=1,nint
         xl(1)=xligs3(idg2(1,intg))
         xl(2)=xligs3(idg2(2,intg))

         xlgp(:,intg)=xl(:)
         wint(intg)=wigs3(idg2(1,intg))*wigs3(idg2(2,intg))

         dndxl(1,1,intg)=(-one+two*xl(1))         &
        &               *(-one+    xl(2))*xl(2)*d4
         dndxl(2,1,intg)=(-one+    xl(1))*xl(1)   &
        &               *(-one+two*xl(2))      *d4
         dndxl(1,2,intg)=( one+two*xl(1))         &
        &               *(-one+    xl(2))*xl(2)*d4
         dndxl(2,2,intg)=( one+    xl(1))*xl(1)   &
        &               *(-one+two*xl(2))      *d4
         dndxl(1,3,intg)=( one+two*xl(1))         &
       &                *( one+    xl(2))*xl(2)*d4
         dndxl(2,3,intg)=( one+    xl(1))*xl(1)   &
        &               *( one+two*xl(2))      *d4
         dndxl(1,4,intg)=(-one+two*xl(1))         &
        &               *( one+    xl(2))*xl(2)*d4
         dndxl(2,4,intg)=(-one+    xl(1))*xl(1)   &
        &               *( one+two*xl(2))      *d4

         dndxl(1,5,intg)=-                    xl(1)&
        &               * (-one+    xl(2)   )*xl(2)
         dndxl(2,5,intg)=-(-one+    xl(1)**2)      &
        &               * (-one+two*xl(2)   )      *d2 
         dndxl(1,6,intg)=-( one+two*xl(1)   )      &
        &               * (-one+    xl(2)**2)      *d2
         dndxl(2,6,intg)=-( one+    xl(1)   )*xl(1)&
        &               *                     xl(2)
         dndxl(1,7,intg)=-                    xl(1)&
        &               * ( one+    xl(2)   )*xl(2)
         dndxl(2,7,intg)=-(-one+    xl(1)**2)      &
        &               * ( one+two*xl(2)   )      *d2
         dndxl(1,8,intg)=-(-one+two*xl(1)   )      &
        &               * (-one+    xl(2)**2)      *d2
         dndxl(2,8,intg)=-(-one+    xl(1)   )*xl(1)&
        &               *                     xl(2)

         dndxl(1,9,intg)=           xl(1)          &
        &               * (-one+    xl(2)**2)*two
         dndxl(2,9,intg)= (-one+    xl(1)**2)      &
        &               *           xl(2)    *two
      end do

!     Surface element

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
