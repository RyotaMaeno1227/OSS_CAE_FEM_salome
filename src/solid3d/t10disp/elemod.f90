! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module mddim

!  problem and modeling specification

   implicit none
   integer,parameter        :: ndim  =3      ! Space dimension of proplem
   integer,parameter        :: nddof =ndim   ! Nodal Degrees of Freedom
   integer,parameter        :: nnde  =10     ! # of nodes for each element

   integer,parameter        :: nsdim =ndim-1 ! Surface dimension of proplem
   integer,parameter        :: nsrf  =6      ! # of nodes for element surface

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

   integer,parameter :: nint =4

   real(kind=DP_REAL_KIND) dndxl (ndim ,nnde ,nint )
   real(kind=DP_REAL_KIND) dndxl0(ndim ,nnde )
   real(kind=DP_REAL_KIND) cnxl  (nnde ,      nint )
   real(kind=DP_REAL_KIND) wint  (            nint )
   real(kind=DP_REAL_KIND) xlgp  (ndim ,      nint )

   integer,parameter :: nsint=3
   real(kind=DP_REAL_KIND) dmdxl (nsdim,nsrf ,nsint)
   real(kind=DP_REAL_KIND) cmxl  (      nsrf ,nsint)
   real(kind=DP_REAL_KIND) wsint (            nsint)

contains

   subroutine initcf

      implicit none

      real(kind=DP_REAL_KIND),dimension(2),parameter :: xlig2 &
  &      =(/ 0.58541020d0, 0.13819660d0 /)

      integer,dimension(ndim,nint),parameter :: idg2             &
  &      =reshape( (/ 2,2,2, 1,2,2, 2,1,2, 2,2,1 /),(/ndim,nint/) )

      real(kind=DP_REAL_KIND),dimension(2),parameter :: xligs &
  &      =(/ 0.6666666666666667d0, 0.1666666666666667d0/)

      real(kind=DP_REAL_KIND),dimension(ndim) :: xl
      real(kind=DP_REAL_KIND)                 :: xlt

      integer                 :: i
      integer                 :: intg

      do intg=1,nint
         do i=1,ndim
            xl(i)=xlig2(idg2(i,intg))
            xlgp(i,intg)=xl(i)
         end do
         xlt=one-xl(1)-xl(2)-xl(3)

         wint(intg)=one/24.0d0

         dndxl(1,1,intg)=one-four*xlt
         dndxl(2,1,intg)=dndxl(1,1,intg)
         dndxl(3,1,intg)=dndxl(1,1,intg)
         dndxl(1,2,intg)=four*xl(1)-one
         dndxl(2,2,intg)=zero
         dndxl(3,2,intg)=zero
         dndxl(1,3,intg)=zero
         dndxl(2,3,intg)=four*xl(2)-one
         dndxl(3,3,intg)=zero
         dndxl(1,4,intg)=zero
         dndxl(2,4,intg)=zero
         dndxl(3,4,intg)=four*xl(3)-one

         dndxl(1,5,intg)=four*(xlt-xl(1))
         dndxl(2,5,intg)=-four*xl(1)
         dndxl(3,5,intg)=dndxl(2,5,intg)
         dndxl(1,6,intg)=four*xl(2)
         dndxl(2,6,intg)=four*xl(1)
         dndxl(3,6,intg)=zero
         dndxl(1,7,intg)=-four*xl(2)
         dndxl(2,7,intg)=four*(xlt-xl(2))
         dndxl(3,7,intg)=dndxl(1,7,intg)

         dndxl(1,8,intg)=-four*xl(3)
         dndxl(2,8,intg)=dndxl(1,8,intg)
         dndxl(3,8,intg)=four*(xlt-xl(3))
         dndxl(1,9,intg)=four*xl(3)
         dndxl(2,9,intg)=zero
         dndxl(3,9,intg)=four*xl(1)
         dndxl(1,10,intg)=zero
         dndxl(2,10,intg)=four*xl(3)
         dndxl(3,10,intg)=four*xl(2)


      end do

      xl=d4
      xlt=d4

      dndxl0(1,1)=one-four*xlt
      dndxl0(2,1)=dndxl0(1,1)
      dndxl0(3,1)=dndxl0(1,1)
      dndxl0(1,2)=four*xl(1)-one
      dndxl0(2,2)=zero
      dndxl0(3,2)=zero
      dndxl0(1,3)=zero
      dndxl0(2,3)=four*xl(2)-one
      dndxl0(3,3)=zero
      dndxl0(1,4)=zero
      dndxl0(2,4)=zero
      dndxl0(3,4)=four*xl(3)-one

      dndxl0(1,5)= four*(xlt-xl(1))
      dndxl0(2,5)=-four*xl(1)
      dndxl0(3,5)= dndxl0(2,5)
      dndxl0(1,6)=four*xl(2)
      dndxl0(2,6)=four*xl(1)
      dndxl0(3,6)=zero
      dndxl0(1,7)=-four*xl(2)
      dndxl0(2,7)= four*(xlt-xl(2))
      dndxl0(3,7)= dndxl0(1,7)

      dndxl0(1,8)=-four*xl(3)
      dndxl0(2,8)= dndxl0(1,8)
      dndxl0(3,8)= four*(xlt-xl(3))
      dndxl0(1,9)=four*xl(3)
      dndxl0(2,9)=zero
      dndxl0(3,9)=four*xl(1)
      dndxl0(1,10)=zero
      dndxl0(2,10)=four*xl(3)
      dndxl0(3,10)=four*xl(2)

!     Surface element

      do intg=1,nsint
         xl(1)=xligs(idg2(1,intg))
         xl(2)=xligs(idg2(2,intg))
         xlt=one-xl(1)-xl(2)

         wsint(intg)=d6

         dmdxl(1,1,intg)=-three+four*xl(1)+four*xl(2)
         dmdxl(2,1,intg)=-three+four*xl(1)+four*xl(2)
         dmdxl(1,2,intg)=four*xl(1)-one
         dmdxl(2,2,intg)=zero
         dmdxl(1,3,intg)=zero
         dmdxl(2,3,intg)=four*xl(2)-one

         dmdxl(1,4,intg)=four-eight*xl(1)-four*xl(2)
         dmdxl(2,4,intg)=-four*xl(1)
         dmdxl(1,5,intg)= four*xl(2)
         dmdxl(2,5,intg)= four*xl(1)
         dmdxl(1,6,intg)=-four*xl(2)
         dmdxl(2,6,intg)=four-four*xl(1)-eight*xl(2)

         cmxl(1,intg)=(two*xlt  -one)*xlt
         cmxl(2,intg)=(two*xl(1)-one)*xl(1)
         cmxl(3,intg)=(two*xl(2)-one)*xl(2)
         cmxl(4,intg)=xlt  *xl(1)*four
         cmxl(5,intg)=xl(1)*xl(2)*four
         cmxl(6,intg)=xl(2)*xlt  *four

      end do

   end subroutine initcf
end module cfmc

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
