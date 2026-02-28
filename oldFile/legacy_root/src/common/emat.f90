! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module matmod

!  elastic material module

   use kind_parameters
   use mddim

   implicit none

   real(kind=DP_REAL_KIND)   :: yom     ! Young's modulus
   real(kind=DP_REAL_KIND)   :: por     ! Poisson's ratio
   real(kind=DP_REAL_KIND)   :: thckns  ! Plate and Shell Thickness
!  D matrix
   real(kind=DP_REAL_KIND),dimension(nstr  ,nstr  ) :: dmat   ! for FEM
   real(kind=DP_REAL_KIND),dimension(nstrss,nstr  ) :: dmatpr ! for Output

contains

   subroutine caldm

!   Calculate D matrix

      use dconst

      implicit none

      integer,parameter :: mxstr = 6

      real(kind=DP_REAL_KIND),dimension(mxstr,mxstr) :: ddmat ! D matrix
      real(kind=DP_REAL_KIND) :: coff,sh,ax

      ddmat=zero
      select case(mtype)
      case(1) 

      !  plane strain

         coff= yom * (one-por)/((one+por)*(one-two*por))
         sh  = coff * (one-two*por)*d2/(one-por)
         ax  = coff * por/(one-por)


         ddmat(1,1)=coff
         ddmat(2,1)=ax
         ddmat(1,2)=ax
         ddmat(2,2)=coff
         ddmat(3,3)=sh
         ddmat(4,1)=ax
         ddmat(4,2)=ax

      case(2)

      !  plane stress

         coff= yom / (one-por**2)
         sh  = yom / (two*(one+por))
         ax  = coff * por
    
    
         ddmat(1,1)=coff
         ddmat(2,1)=ax
         ddmat(1,2)=ax
         ddmat(2,2)=coff
         ddmat(3,3)=sh

      case(4)

      !  plate ( Transverse shear is considered )

         coff= yom*(thckns**3)/(12.0d0*(one-por**2))
         ax  = coff * por
         sh  = yom/(two*(one+por))*thckns *5.0d0/6.0d0
  
         ddmat(1,1)=coff
         ddmat(2,1)=ax
         ddmat(1,2)=ax
         ddmat(2,2)=coff
         ddmat(3,3)=(one-por)*d2*coff
         ddmat(4,4)=sh
         ddmat(5,5)=sh

      case(5)

      !  plate ( Transverse shear is not considered )

         coff= yom*(thckns**3)/(12.0d0*(one-por**2))
         ax  = coff * por

         ddmat(1,1)=coff
         ddmat(2,1)=ax
         ddmat(1,2)=ax
         ddmat(2,2)=coff
         ddmat(3,3)=(one-por)*d2*coff

      case default

      !  3D solid

         coff= yom * (one-por)/((one+por)*(one-two*por))
         sh  = coff * (one-two*por)*d2/(one-por)
         ax  = coff * por/(one-por)
  
         ddmat(1,1)=coff
         ddmat(2,1)=ax
         ddmat(3,1)=ax
         ddmat(1,2)=ax
         ddmat(2,2)=coff
         ddmat(3,2)=ax
         ddmat(1,3)=ax
         ddmat(2,3)=ax
         ddmat(3,3)=coff
         ddmat(4,4)=sh
         ddmat(5,5)=sh
         ddmat(6,6)=sh

      end select

      dmat  =ddmat(1:nstr  ,1:nstr)
      dmatpr=ddmat(1:nstrss,1:nstr)

   end subroutine caldm

end module matmod

module bpload

!  Data module for body force and pressure

   use kind_parameters
   use mddim

   real(kind=DP_REAL_KIND),dimension(ndim  ) :: bdyf  ! Body force vector
   real(kind=DP_REAL_KIND)                   :: prssr ! Pressure

end module bpload

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
