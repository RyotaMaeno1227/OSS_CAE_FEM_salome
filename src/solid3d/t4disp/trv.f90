! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module trv

   use mddim
   use kind_parameters

   implicit none

   integer                                     :: ntrs
!                                                 # of Surface Element
   integer,allocatable,dimension(:,:)          :: idtrn
!                                                 Index of Nodes describing
!                                                 traction-prescribed surface

   real(kind=DP_REAL_KIND),allocatable,dimension(:,:) :: trvse
!                                                 Traction force vector

contains

   subroutine initrv

      implicit none

      allocate(idtrn (nsrf  ,ntrs  ))
      allocate(trvse (ndim  ,ntrs  ))

   end subroutine initrv

   subroutine ctrv

!     Calculate Traction force vector

      use dconst
      use solmod
      use cfmc

      implicit none

      real(kind=DP_REAL_KIND) :: xel   (ndim  ,nsrf  )
      real(kind=DP_REAL_KIND),dimension(ndim  ,nsdim ) :: sv
      real(kind=DP_REAL_KIND),dimension(ndim  ) :: onv
      real(kind=DP_REAL_KIND),dimension(nsrf  ) :: cmtr
      real(kind=DP_REAL_KIND),dimension(ndim  ,nsrf  ) :: evtr
      real(kind=DP_REAL_KIND)                   :: cas

      integer          i,j,k
      integer          jn
      integer          ijd
      integer          is,intg

      do is=1,ntrs

         do i=1,nsrf
            j=idtrn(i,is)
            xel(:,i)=xnode(:,j)
         end do

         cmtr=zero
         do intg=1,nsint

            sv=zero
            do i=1,nsrf
               do j=1,nsdim
                  do k=1,ndim
                     sv(k,j)=sv(k,j)+dmdxl(j,i,intg)*xel(k,i)
                  end do
               end do
            end do

            onv(1)=sv(2,1)*sv(3,2)-sv(3,1)*sv(2,2)
            onv(2)=sv(3,1)*sv(1,2)-sv(1,1)*sv(3,2)
            onv(3)=sv(1,1)*sv(2,2)-sv(2,1)*sv(1,2)
            cas=wsint(intg)*sqrt(dot_product(onv,onv))

            do i=1,nsrf
               cmtr(i)=cmtr(i)+cas*cmxl(i,intg)
            end do
         end do

!        print *,'cmtr',cmtr
         do i=1,nsrf
            evtr(:,i)=trvse(:,is)*cmtr(i)
         end do
!        print *,'evtr'
!        print *,evtr

         do j=1,nsrf
            jn=idtrn(j,is)
            do i=1,ndim
               ijd=idnd(i,jn)
               if(ijd.gt.0) then
                  fvec(ijd)=fvec(ijd)+evtr(i,j)
               end if
            end do
         end do
      end do

   end subroutine ctrv

end module trv

subroutine cbpv

!  Calculate nodal vector for Body force or Pressure

   use nedata
   use bpload
   use cfmc
   use solmod
   use dconst

   implicit none

   real(kind=DP_REAL_KIND) :: xel   (ndim  ,nnde  )
   real(kind=DP_REAL_KIND) :: dxdxl (ndim  ,ndim  )
   real(kind=DP_REAL_KIND) :: dxldx (ndim  ,ndim  )
   real(kind=DP_REAL_KIND) :: bfnv  (ndim  ,nnde  )

   real(kind=DP_REAL_KIND) :: svol
   real(kind=DP_REAL_KIND) :: vol

   integer          jelm,intg
   integer          i,j,k
   integer          ij

   if(dot_product(bdyf,bdyf).eq.zero) return

   do jelm=1,nelem

      bfnv=zero
      do i=1,nnde
         j=idelm(i,jelm)
         xel(:,i)=xnode(:,j)
      end do

      do intg=1,nint
         do k=1,ndim
            do i=1,ndim
               dxdxl(k,i)=dot_product(dndxl(i,:,intg),xel(k,:))
            end do
         end do

         call sminv(ndim,dxdxl,svol,dxldx)

         vol=svol*wint(intg)

         do j=1,nnde
            do i=1,ndim
               bfnv(i,j)=bfnv(i,j)+cnxl(i,intg)*bdyf(i)*vol
            end do
         end do

      end do

      do k=1,nnde
         j=idelm(k,jelm)
         do i=1,ndim
            ij=idnd(i,j)
            if(ij.gt.0) then
              fvec(ij)=fvec(ij)+bfnv(i,k)
            end if
         end do
      end do
   end do

end subroutine cbpv

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
