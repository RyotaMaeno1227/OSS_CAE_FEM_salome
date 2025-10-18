! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

subroutine calfv

!     update variable vector

      use nedata
      use solmod
      use trv
      use dconst

      implicit none

      integer          i,j
      integer          ijd

      do j=1,nnode
         do i=1,nddof
            ijd=idnd(i,j)
            if(ijd.gt.0) then
               fvec(ijd)=fvec(ijd)+fnode(i,j)
            end if
         end do
      end do

      call ctrv
      call cbpv

end subroutine calfv

subroutine udvec

!     update variable vector

      use nedata
      use solmod
      use dconst

      implicit none

      integer          i,j

!     print *,((idnd(i,j),vnode(i,j),i=1,ndim),j=1,nnode)
      do j=1,nnode
         do i=1,nddof
            if(idnd(i,j).gt.0) then
               unode(i,j)=unew(idnd(i,j))
            end if
         end do
      end do

end subroutine udvec

subroutine calfi

!  Calculate Internal Force

   use nedata
   use dconst

   implicit none

   real(kind=DP_REAL_KIND) :: stfe (nndof,nndof)
   real(kind=DP_REAL_KIND) :: uel  (nndof)
   real(kind=DP_REAL_KIND) :: fintl(nndof)

   integer                 :: jelm
   integer                 :: i,j,k
   integer                 :: ii

   fint=zero
   do jelm=1,nelem

      call calstf(stfe  ,jelm )

      do i=1,nnde
         j=idelm(i,jelm)
         ii=(i-1)*nddof
         do k=1,nddof
            uel(ii+k)=unode(k,j)
         end do
      end do
      fintl=matmul(stfe,uel)

      do i=1,nnde
         j=idelm(i,jelm)
         ii=(i-1)*nddof
         do k=1,nddof
            fint(k,j)=fint(k,j)+fintl(ii+k)
         end do
      end do

   end do

end subroutine calfi

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
