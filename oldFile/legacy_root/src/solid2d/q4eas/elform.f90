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

   integer,parameter :: nires=1

   real(kind=DP_REAL_KIND),dimension(ndim ,nires) :: xlspr

   character(len=13),parameter::eletyp='Quadrilateral'

contains

   subroutine setxlp

      implicit none

      xlspr=0.0d0

   end subroutine setxlp

end module upelm

module srelm

   use upelm
   use cfmc
   use matmod

!  module for Simo Rifai element

   integer,parameter :: nstpr=4

   real(kind=DP_REAL_KIND),dimension(nstr ,nstr ) :: f0inv
   real(kind=DP_REAL_KIND) :: xj0

   real(kind=DP_REAL_KIND),dimension(nstpr,nstpr) :: hmat
   real(kind=DP_REAL_KIND),dimension(nstpr,nndof) :: gmat

contains

   subroutine bigcal

   !  calculate geometrical data before numerical integration

      use dconst
  
      implicit none
  
      real(kind=DP_REAL_KIND) :: dmmy
      real(kind=DP_REAL_KIND),dimension(nstr ,nstr ) :: f0

      integer i,j

      do j=1,ndim
         do i=1,ndim
            dxdxl(j,i)=dot_product(dndxl0(i,:),xel(j,:))
         end do
      end do

      xj0=dxdxl(1,1)*dxdxl(2,2)-dxdxl(1,2)*dxdxl(2,1)

      f0(1,1)=dxdxl(1,1)**2
      f0(1,2)=dxdxl(1,2)*dxdxl(1,2)
      f0(1,3)=dxdxl(1,1)*dxdxl(1,2)*two
      f0(2,1)=dxdxl(2,1)*dxdxl(2,1)
      f0(2,2)=dxdxl(2,2)**2
      f0(2,3)=dxdxl(2,1)*dxdxl(2,2)*two
      f0(3,1)=dxdxl(1,1)*dxdxl(2,1)
      f0(3,2)=dxdxl(1,2)*dxdxl(2,2)
      f0(3,3)=dxdxl(1,1)*dxdxl(2,2)+dxdxl(1,2)*dxdxl(2,1)

      call sminv(nstr,f0,dmmy,f0inv)

      hmat=zero
      gmat=zero

   end subroutine bigcal

   subroutine aicst(stfe)

   !  calculate stiffness matrix after numerical integration

      use dconst
  
      implicit none

      real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)

      real(kind=DP_REAL_KIND),dimension(nstpr,nndof) :: hig
      integer                ,dimension(nstpr)       :: ip

      integer          i,j

      call pludec(nstpr,hmat,hig(:,1),ip)
      do i=1,nndof
         call plusol(nstpr,hmat,gmat(:,i),hig(:,i),ip)
      end do

      do j=1,nndof
         do i=1,nndof
            stfe(i,j)=stfe(i,j)-dot_product(gmat(:,i),hig(:,j))
         end do
      end do

   end subroutine aicst

end module srelm

subroutine shcoef(jelm)

!  set element data

   use nedata
   use upelm
   use cfmc
   use dconst
   use srelm

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

subroutine cestf(stfe)

!  calculate element stiffness matrix

   use upelm
   use dconst
   use srelm

   implicit none

   real(kind=DP_REAL_KIND),intent(out) :: stfe (nndof,nndof)

   integer          i,j
   integer          j1,j2

   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: dbmat
   real(kind=DP_REAL_KIND),dimension(nstr,nstpr) :: emat
   real(kind=DP_REAL_KIND),dimension(nstr,nstpr) :: gsm
   real(kind=DP_REAL_KIND),dimension(nstr,nstpr) :: dgsm
   real(kind=DP_REAL_KIND) :: cj

!  displacement model

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

!  enhanced term

   emat=zero
   emat(1,1)= xlgp(1,intg)
!  emat(1,5)= xlgp(1,intg)*xlgp(2,intg)
   emat(2,2)= xlgp(2,intg)
!  emat(2,5)=-xlgp(1,intg)*xlgp(2,intg)
   emat(3,3)= xlgp(1,intg)
   emat(3,4)= xlgp(2,intg)
!  emat(3,5)= xlgp(1,intg)**2-xlgp(2,intg)**2

   cj=xj0/svol

   do j=1,nstpr
      do i=1,nstr
         gsm(i,j)=dot_product(f0inv(:,i),emat(:,j))*cj
      end do
   end do

   do j=1,nndof
      do i=1,nstpr
         gmat(i,j)=gmat(i,j)+dot_product(gsm(:,i),dbmat(:,j))*vol
      end do
   end do

   do j=1,nstpr
      do i=1,nstr
         dgsm(i,j)=dot_product(dmat(i,:),gsm(:,j))
      end do
   end do

   do j=1,nstpr
      do i=1,nstpr
         hmat(i,j)=hmat(i,j)+dot_product(gsm(:,i),dgsm(:,j))*vol
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

   real(kind=DP_REAL_KIND),dimension(nstr,nires),intent(out) :: stress
   integer                                      ,intent(in ) :: jelm

   real(kind=DP_REAL_KIND) :: uel(nndof )
   real(kind=DP_REAL_KIND),dimension(nstr ) :: strain
   integer          i,j,k

   do i=1,nnde
      j=idelm(i,jelm)
      do k=1,ndim
         uel(nddof*(i-1)+k)=unode(k,j)
      end do
      xel(:,i)=xnode(:,j)
   end do

   strain=zero
   tvol  =zero
   do intg=1,nint

      call shcoef(jelm)
      tvol  =tvol+vol
      do i=1,nstr
         strain(i)=strain(i)+dot_product(bmat(i,:),uel)*vol
      end do
   end do
   strain=strain/tvol
   stress(:,1)=matmul(dmatpr,strain)

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
