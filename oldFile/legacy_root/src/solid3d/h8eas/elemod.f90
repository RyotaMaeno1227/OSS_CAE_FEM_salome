! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =3      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =8      ! # of nodes for each element

   integer,parameter        :: nsdim =ndim-1 ! Surface dimension of proplem
   integer,parameter        :: nsrf  =4      ! # of nodes for element surface

   integer,parameter        :: nndof =nnde*nddof
   integer,parameter        :: nstr  =6      ! Number of Strain Component

   integer,parameter        :: mtype =3      ! Material type
   integer,parameter        :: nstrss=6      ! Number of Stress Component

   character(len=3),parameter :: letyp ='hex' ! Element type name

end module mddim

module cfmc

   use dconst
   use mddim

   implicit none

   integer,parameter :: nint=8

   real(kind=DP_REAL_KIND) dndxl (ndim ,nnde ,nint )
   real(kind=DP_REAL_KIND) dndxl0(ndim ,nnde )
   real(kind=DP_REAL_KIND) cnxl  (nnde ,      nint )
   real(kind=DP_REAL_KIND) wint  (            nint )
   real(kind=DP_REAL_KIND) xlgp  (ndim ,      nint )

   integer,parameter :: nsint=4
   real(kind=DP_REAL_KIND) dmdxl (nsdim,nsrf ,nsint)
   real(kind=DP_REAL_KIND) cmxl  (      nsrf ,nsint)
   real(kind=DP_REAL_KIND) wsint (            nsint)

contains

   subroutine initcf

      implicit none

      real(kind=DP_REAL_KIND),dimension(2) :: xlig2

      integer,dimension(ndim,nint),parameter :: idg2             &
  &      =reshape( (/ 1,1,1, 1,2,1, 2,1,1, 2,2,1,                &
  &                   1,1,2, 1,2,2, 2,1,2, 2,2,2/),(/ndim,nint/) )

      real(kind=DP_REAL_KIND),dimension(ndim,nnde),parameter :: xln&
     &   =reshape( (/ -1.0,-1.0,-1.0,  1.0,-1.0,-1.0,  &
     &                 1.0, 1.0,-1.0, -1.0, 1.0,-1.0,  &
     &                -1.0,-1.0, 1.0,  1.0,-1.0, 1.0,  &
     &                 1.0, 1.0, 1.0, -1.0, 1.0, 1.0/),&
     &             (/ndim,nnde/) )

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl
      integer                 :: i
      integer                 :: intg

      xlig2(1)=-1.0d0/sqrt(3.0d0)
      xlig2(2)= 1.0d0/sqrt(3.0d0)

      do intg=1,nint
         do i=1,ndim
            xl(i)=xlig2(idg2(i,intg))
           xlgp(i,intg)=xl(i)
         end do

         wint(intg)=one

         do i=1,nnde
            dndxl(1,i,intg)=xln(1,i)               &
           &               *(one+xln(2,i)*xl(2))   &
           &               *(one+xln(3,i)*xl(3))*d8
            dndxl(2,i,intg)=(one+xln(1,i)*xl(1))   &
           &               *xln(2,i)               &
           &               *(one+xln(3,i)*xl(3))*d8
            dndxl(3,i,intg)=(one+xln(1,i)*xl(1))   &
           &               *(one+xln(2,i)*xl(2))   &
           &               *xln(3,i)            *d8

            cnxl (  i,intg)=(one+xln(1,i)*xl(1))   & 
           &               *(one+xln(2,i)*xl(2))   &
           &               *(one+xln(3,i)*xl(3))*d8
         end do
      end do

      do i=1,nnde
         dndxl0(1,i)=xln(1,i)*d8
         dndxl0(2,i)=xln(2,i)*d8
         dndxl0(3,i)=xln(3,i)*d8
      end do

!     Surface element

      do intg=1,nsint
         xl(1)=xlig2(idg2(1,intg))
         xl(2)=xlig2(idg2(2,intg))

         wsint(intg)=one

         dmdxl(1,1,intg)=-one       *(one-xl(2))*d4
         dmdxl(2,1,intg)=(one-xl(1))*(-one)     *d4
         dmdxl(1,2,intg)= one       *(one-xl(2))*d4
         dmdxl(2,2,intg)=(one+xl(1))*(-one)     *d4
         dmdxl(1,3,intg)= one       *(one+xl(2))*d4
         dmdxl(2,3,intg)=(one+xl(1))* one       *d4
         dmdxl(1,4,intg)=-one       *(one+xl(2))*d4
         dmdxl(2,4,intg)=(one-xl(1))* one       *d4

         cmxl(1,intg)=(one-xl(1))*(one-xl(2))*d4
         cmxl(2,intg)=(one+xl(1))*(one-xl(2))*d4
         cmxl(3,intg)=(one+xl(1))*(one+xl(2))*d4
         cmxl(4,intg)=(one-xl(1))*(one+xl(2))*d4

      end do

   end subroutine initcf

end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
