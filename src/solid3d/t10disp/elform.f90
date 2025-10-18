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

   integer,parameter :: nires=4

   real(kind=DP_REAL_KIND),dimension(ndim ,nires) :: xlspr

   character(len=10),parameter::eletyp='Tetrahedra'

contains

   subroutine setxlp

      use cfmc

      implicit none

      xlspr=xlgp

   end subroutine setxlp

end module upelm

subroutine shcoef(jelm)

!  set element data

   use nedata
   use upelm
   use cfmc
   use dconst

   implicit none

   integer,intent(in) :: jelm

   integer          j,jj
   integer          ix,kx
   integer          jnd

   do kx=1,ndim
      do ix=1,ndim
         dxdxl(kx,ix)=dot_product(dndxl(ix,:,intg),xel(kx,:))
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

   do jnd=1,nnde
      do ix=1,ndim
         dndx(ix,jnd)=dot_product(dxldx(:,ix),dndxl(:,jnd,intg))
      end do
   end do

   bmat =zero
   do j=1,nnde
      jj=nddof*(j-1)
      bmat(1,jj+1)=dndx(1,j)
      bmat(2,jj+2)=dndx(2,j)
      bmat(3,jj+3)=dndx(3,j)
      bmat(4,jj+1)=dndx(2,j)
      bmat(4,jj+2)=dndx(1,j)
      bmat(5,jj+2)=dndx(3,j)
      bmat(5,jj+3)=dndx(2,j)
      bmat(6,jj+3)=dndx(1,j)
      bmat(6,jj+1)=dndx(3,j)
   end do

end subroutine shcoef

subroutine cestf(stfe)

!  calculate element stiffness matrix

   use upelm
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)

   integer          i,j
   integer          j1,j2

   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: dbmat

   do j=1,nndof
      do i=1,nstr
         dbmat(i,j)=dot_product(dmat(i,:),bmat(:,j))
      end do
   end do

   do j2=1,nndof
      do j1=1,nndof
         stfe(j1,j2)=stfe(j1,j2)                            &
        &           +dot_product(bmat(:,j1),dbmat(:,j2))*vol
      end do
   end do

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
      do k=1,ndim
         uel(nddof*(i-1)+k)=unode(k,j)
      end do
      xel(:,i)=xnode(:,j)
   end do

   do intg=1,nint

      call shcoef(jelm)
      do i=1,nstr
      end do
      strain        =matmul(bmat,uel)
      stress(:,intg)=matmul(dmatpr,strain)
   end do

end subroutine css


! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
