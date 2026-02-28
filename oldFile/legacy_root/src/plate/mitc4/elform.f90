! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module upelm

   use nedata

   implicit none

   integer          intg

   real(kind=DP_REAL_KIND) :: xel   (ndim  ,nnde  )

   real(kind=DP_REAL_KIND) :: dxdxl (ndim ,ndim )
                            !  Jaconbian matrix
   real(kind=DP_REAL_KIND) :: dxldx (ndim ,ndim )
   real(kind=DP_REAL_KIND) :: svol
   real(kind=DP_REAL_KIND) :: vol

   real(kind=DP_REAL_KIND) :: dndx (ndim  ,nnde  )
   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: bmat

!  real(kind=DP_REAL_KIND) :: fve   (ndim  ,nelnd )

   real(kind=DP_REAL_KIND) :: fe   (nndof)

   integer,parameter :: nires=4

   real(kind=DP_REAL_KIND),dimension(ndim ,nires) :: xlspr

   character(len=13),parameter::eletyp='Quadrilateral'

contains

   subroutine setxlp

      use cfmc

      implicit none

      xlspr=xlgp

   end subroutine setxlp

end module upelm

subroutine shcoef(jelm  )

!  set element data

   use nedata
   use upelm
   use cfmc
   use dconst

   implicit none

   integer,intent(in ) :: jelm

   real(kind=DP_REAL_KIND),dimension(ndim ,4   ) :: cgm
   real(kind=DP_REAL_KIND),dimension(ndim      ) :: xl

   integer          i,j,k
   integer          ix,kx
   
   dxdxl=zero
   do kx=1,ndim
      do ix=1,ndim
         do j=1,nnde
            dxdxl(kx,ix)=dxdxl(kx,ix)+dndxl(ix,j,intg)*xel(kx,j)
         end do
      end do
   end do

   call sminv(ndim,dxdxl,svol,dxldx)

   if(svol.le.0) then
      print *,"J<0",jelm
      write(*,'(1h ,3g12.3)') dxdxl
      print *,'xel' 
      write(*,'(1h ,3g12.3)') xel
   end if
   vol=svol*wint(intg)

   dndx=matmul(transpose(dxldx),dndxl(:,:,intg))

   xl(:)=xlgp(:,intg)

   bmat =zero
   do i=1,nnde
      j=nddof*(i-1)
      bmat(1,j+3)= dndx(1,i)
      bmat(2,j+2)=-dndx(2,i)
      bmat(3,j+2)=-dndx(1,i)
      bmat(3,j+3)= dndx(2,i)
   end do

   do k=1,ndim
      cgm(k,1)=dxldx(1,k)*(one-xl(2))*d4
      cgm(k,2)=dxldx(1,k)*(one+xl(2))*d4
      cgm(k,3)=dxldx(2,k)*(one-xl(1))*d4
      cgm(k,4)=dxldx(2,k)*(one+xl(1))*d4
   end do

   do j=1,ndim
      bmat(j+3, 1)=-cgm(j,1)
      bmat(j+3, 2)=-cgm(j,1)*(xel(2,2)-xel(2,1))*d2
      bmat(j+3, 3)= cgm(j,1)*(xel(1,2)-xel(1,1))*d2
      bmat(j+3, 4)= cgm(j,1)
      bmat(j+3, 5)=-cgm(j,1)*(xel(2,2)-xel(2,1))*d2
      bmat(j+3, 6)= cgm(j,1)*(xel(1,2)-xel(1,1))*d2
      bmat(j+3, 7)= cgm(j,2)
      bmat(j+3, 8)=-cgm(j,2)*(xel(2,3)-xel(2,4))*d2
      bmat(j+3, 9)= cgm(j,2)*(xel(1,3)-xel(1,4))*d2
      bmat(j+3,10)=-cgm(j,2)
      bmat(j+3,11)=-cgm(j,2)*(xel(2,3)-xel(2,4))*d2
      bmat(j+3,12)= cgm(j,2)*(xel(1,3)-xel(1,4))*d2
      bmat(j+3, 1)=-cgm(j,3)                       +bmat(j+3, 1)
      bmat(j+3, 2)=-cgm(j,3)*(xel(2,4)-xel(2,1))*d2+bmat(j+3, 2)
      bmat(j+3, 3)= cgm(j,3)*(xel(1,4)-xel(1,1))*d2+bmat(j+3, 3)
      bmat(j+3,10)= cgm(j,3)                       +bmat(j+3,10)
      bmat(j+3,11)=-cgm(j,3)*(xel(2,4)-xel(2,1))*d2+bmat(j+3,11)
      bmat(j+3,12)= cgm(j,3)*(xel(1,4)-xel(1,1))*d2+bmat(j+3,12)
      bmat(j+3, 4)=-cgm(j,4)                       +bmat(j+3, 4)
      bmat(j+3, 5)=-cgm(j,4)*(xel(2,3)-xel(2,2))*d2+bmat(j+3, 5)
      bmat(j+3, 6)= cgm(j,4)*(xel(1,3)-xel(1,2))*d2+bmat(j+3, 6)
      bmat(j+3, 7)= cgm(j,4)                       +bmat(j+3, 7)
      bmat(j+3, 8)=-cgm(j,4)*(xel(2,3)-xel(2,2))*d2+bmat(j+3, 8)
      bmat(j+3, 9)= cgm(j,4)*(xel(1,3)-xel(1,2))*d2+bmat(j+3, 9)
   end do

!  print *,'bmat'
!  write(*,'(1h ,5g12.3)' ) bmat

end subroutine shcoef

subroutine cestf(stfe)

!  calculate element stiffness matrix

   use upelm
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)

   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: dbmat

   dbmat=matmul(dmat,bmat)

   stfe=stfe+matmul(transpose(bmat),dbmat)*vol

end subroutine cestf

subroutine css(stress,jelm)

!  calculate strain and stress

   use upelm
   use cfmc
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),dimension(nstrss,nires),intent(out) :: stress
   integer                                        ,intent(in ) :: jelm

   real(kind=DP_REAL_KIND),dimension(nndof ) :: uel
   real(kind=DP_REAL_KIND),dimension(nstr  ) :: strain

   integer          i,j,k

   do i=1,nnde
      j=idelm(i,jelm)
      do k=1,nddof
         uel(nddof*(i-1)+k)=unode(k,j)
      end do
      xel(:,i)=xnode(:,j)
   end do

   do intg=1,nint
      call shcoef(jelm)
      strain=matmul(bmat,uel)
      stress(:,intg)=matmul(dmatpr,strain)
   end do

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
