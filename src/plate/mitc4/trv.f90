! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module trv

   use nedata

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

      implicit none

      real(kind=DP_REAL_KIND) :: xel   (ndim  ,ndim  )

      real(kind=DP_REAL_KIND),dimension(ndim  ) :: tv,evtr
      real(kind=DP_REAL_KIND)                   :: cas

      integer          i,j
      integer          jn
      integer          ijd
      integer          is

      if(ntrs.eq.0) return

      do is=1,ntrs

         do i=1,nsrf
            j=idtrn(i,is)
            xel(:,i)=xnode(:,j)
         end do

         tv(:)=xel(:,2)-xel(:,1)
         cas=d2*dsqrt(dot_product(tv,tv))

         evtr(:)=trvse(:,is)*cas

         do j=1,nsrf
            jn=idtrn(j,is)
            do i=1,ndim
               ijd=idnd(i,jn)
               if(ijd.gt.0) then
                  fvec(ijd)=fvec(ijd)+evtr(i)
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
   real(kind=DP_REAL_KIND) :: prnv  (nnde  )

   real(kind=DP_REAL_KIND) :: svol
   real(kind=DP_REAL_KIND) :: vol

   integer          jelm,intg
   integer          i,j,k

   if(prssr.eq.zero) return

   do jelm=1,nelem

      prnv=zero
      do i=1,nnde
         j=idelm(i,jelm)
         xel(:,i)=xnode(:,j)
      end do

      do intg=1,nint
         dxdxl=zero
         do k=1,ndim
            do i=1,ndim
               do j=1,nnde
                  dxdxl(k,i)=dxdxl(k,i)+dndxl(i,j,intg)*xel(k,j)
               end do
            end do
         end do

         call sminv(ndim,dxdxl,svol,dxldx)

         vol=svol*wint(intg)

         do i=1,nnde
            prnv(i)=prnv(i)+cnxl(i,intg)*prssr*vol
         end do

      end do

      do i=1,nnde
         j=idelm(i,jelm)
         k=idnd(1,j)
         if(k.gt.0) then
              fvec(k)=fvec(k)+prnv(i)
         end if
      end do
   end do

end subroutine cbpv

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
