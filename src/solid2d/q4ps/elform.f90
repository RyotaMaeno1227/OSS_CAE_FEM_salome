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
   real(kind=DP_REAL_KIND) :: tvol

   real(kind=DP_REAL_KIND) :: dndx (ndim  ,nnde  )
   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: bmat

!  real(kind=DP_REAL_KIND) :: fve   (ndim  ,nelnd )


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

module pselm

   use upelm
   use cfmc
   use matmod

!  module for Pian Sumihara element

   integer,parameter :: nstpr=2

   real(kind=DP_REAL_KIND),dimension(nstpr,nstpr) :: amat
   real(kind=DP_REAL_KIND),dimension(nstpr,nndof) :: cmat
   real(kind=DP_REAL_KIND),dimension(nstr ,nstr  ):: dminv
   real(kind=DP_REAL_KIND),dimension(nstr ,nstpr) :: esn
   real(kind=DP_REAL_KIND),dimension(nstr ,nstpr) :: ccesn
   real(kind=DP_REAL_KIND),dimension(ndim)        :: xls

   real(kind=DP_REAL_KIND),dimension(nstpr,nndof) :: aic

   real(kind=DP_REAL_KIND) :: xj0,xj1,xj2

!  real(kind=DP_REAL_KIND),dimension(nndof) :: vvv
!  real(kind=DP_REAL_KIND),dimension(2) :: ssii

contains

   subroutine bigcal

   !  calculate geometrical data before numerical integration

      use dconst
  
      implicit none
  
      real(kind=DP_REAL_KIND) :: a1,b1,a3,b3
      real(kind=DP_REAL_KIND) :: dmmy
 
!     vvv=reshape(xel,(/nndof/))
!     ssii=zero

      call sminv(nstr,dmat,dmmy,dminv)
  
      a1=dot_product(xlnode(1,:),xel(1,:))
      b1=dot_product(xlnode(1,:),xel(2,:))
      a3=dot_product(xlnode(2,:),xel(1,:))
      b3=dot_product(xlnode(2,:),xel(2,:))

      xj0=(-xel(1,2)*xel(2,1)+xel(1,4)*xel(2,1)    &
         & +xel(1,1)*xel(2,2)-xel(1,3)*xel(2,2)    &
         & +xel(1,2)*xel(2,3)-xel(1,4)*xel(2,3)    &
         & -xel(1,1)*xel(2,4)+xel(1,3)*xel(2,4))*d8
  
      xj1=( xel(1,3)*xel(2,1)-xel(1,4)*xel(2,1)     &
        &  -xel(1,3)*xel(2,2)+xel(1,4)*xel(2,2)     &
        &  -xel(1,1)*xel(2,3)+xel(1,2)*xel(2,3)     &
        &  +xel(1,1)*xel(2,4)-xel(1,2)*xel(2,4))*d8

      xj2=( xel(1,2)*xel(2,1)-xel(1,3)*xel(2,1)    &
         & -xel(1,1)*xel(2,2)+xel(1,4)*xel(2,2)    &
         & +xel(1,1)*xel(2,3)-xel(1,4)*xel(2,3)    &
         & -xel(1,2)*xel(2,4)+xel(1,3)*xel(2,4))*d8
  
      ccesn(1,1)=a3**2
      ccesn(1,2)=a1**2
      ccesn(2,1)=b3**2
      ccesn(2,2)=b1**2
      ccesn(3,1)=a3*b3
      ccesn(3,2)=a1*b1
  
      xls(1)=xj1/(xj0*3.0d0)
      xls(2)=xj2/(xj0*3.0d0)
!     xls=zero

      amat=zero
      cmat=zero
      tvol=zero

   end subroutine bigcal

   subroutine aicst(stfe)

   !  calculate stiffness matrix after numerical integration

      use dconst
  
      implicit none

      real(kind=DP_REAL_KIND),intent(out) :: stfe (nndof,nndof)

      real(kind=DP_REAL_KIND),dimension(nstr ,nndof) :: dbmat
      real(kind=DP_REAL_KIND),dimension(nstpr,nstpr) :: aminv
      real(kind=DP_REAL_KIND),dimension(nndof,nndof) :: estf
      integer          i,j
      integer          jj
      integer          ix,jnd

      do j=1,ndim
         do i=1,ndim
            dxdxl(j,i)=dot_product(dndxl0(i,:),xel(j,:))
         end do
      end do
  
      call sminv(ndim,dxdxl,svol,dxldx)
  
      do jnd=1,nnde
         do ix=1,ndim
            dndx(ix,jnd)=dot_product(dxldx(:,ix),dndxl0(:,jnd))
         end do
      end do
      vol=svol*four

      bmat =zero
      do j=1,nnde
         jj=nddof*(j-1)
         bmat(1,jj+1)=dndx(1,j)
         bmat(2,jj+2)=dndx(2,j)
         bmat(3,jj+1)=dndx(2,j)
         bmat(3,jj+2)=dndx(1,j)
      end do

      dbmat=matmul(dmat,bmat)
      
      stfe =matmul(transpose(bmat),dbmat)*vol

      call sminv(nstpr,amat,svol,aminv)

      aic=matmul(aminv,cmat)
 
      do j=1,nndof
         do i=1,nndof
            estf(i,j)=cmat(1,i)*aic(1,j)+cmat(2,i)*aic(2,j)
            stfe(i,j)=stfe(i,j)+cmat(1,i)*aic(1,j)+cmat(2,i)*aic(2,j)
         end do
      end do

   end subroutine aicst

end module pselm

subroutine shcoef(jelm)

!  set element data

   use nedata
   use upelm
   use cfmc
   use dconst
   use pselm

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
      bmat(3,jj+1)=dndx(2,j)
      bmat(3,jj+2)=dndx(1,j)
   end do

end subroutine shcoef

subroutine cestf

!  calculate element stiffness matrix

   use upelm
   use dconst
   use pselm

   implicit none

   integer          i
   integer          j1,j2

   real(kind=DP_REAL_KIND),dimension(nstr,nstpr) :: dnmat

   do i=1,nstr
      esn(i,1)=ccesn(i,1)*(xlgp(1,intg)-xls(1))
      esn(i,2)=ccesn(i,2)*(xlgp(2,intg)-xls(2))
   end do 

   dnmat=matmul(dminv,esn)

   do j2=1,nstpr
      do j1=1,nstpr
         amat(j1,j2)=amat(j1,j2)                            &
        &           +dot_product(esn(:,j1),dnmat(:,j2))*vol
      end do
   end do

   do j2=1,nndof
      do j1=1,nstpr
         cmat(j1,j2)=cmat(j1,j2)                &
        &           +( esn(1,j1)*bmat(1,j2)     &
        &             +esn(2,j1)*bmat(2,j2)     &
        &             +esn(3,j1)*bmat(3,j2))*vol
      end do
   end do
   tvol=tvol+vol

end subroutine cestf

subroutine css(stress,jelm)

!  calculate strain and stress

   use upelm
   use pselm
   use cfmc
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),dimension(nstrss,nires ),intent(out) :: stress
   integer                                         ,intent(in ) :: jelm

   real(kind=DP_REAL_KIND),dimension(nndof )      :: uel
   real(kind=DP_REAL_KIND),dimension(nstr  )      :: strain,strss0

   real(kind=DP_REAL_KIND),dimension(nndof,nndof) :: stfe 
   real(kind=DP_REAL_KIND),dimension(nstpr)       :: beta

   integer          i,j,k

   do i=1,nnde
      j=idelm(i,jelm)
      do k=1,ndim
         uel(nddof*(i-1)+k)=unode(k,j)
      end do
      xel(:,i)=xnode(:,j)
   end do

   call bigcal
   stfe=zero

   do intg=1,nint

      call shcoef(jelm)

      call cestf

   end do

   call aicst(stfe)

   strain=matmul(bmat,uel)

   do i=1,nstr
      strss0(i)=dot_product(dmat(i,:),strain)
   end do

   beta=matmul(aic,uel)

   do intg=1,nires
      do i=1,nstr
         stress(i,intg)=strss0(i)                               &
        &              +ccesn(i,1)*(xlgp(1,intg)-xls(1))*beta(1)&
        &              +ccesn(i,2)*(xlgp(2,intg)-xls(2))*beta(2)
      end do 
   end do 

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
