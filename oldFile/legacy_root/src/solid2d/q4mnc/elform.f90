! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module upelm

   use nedata
   use cfmc

   implicit none

   integer,parameter :: ntdof=(nnde+nncm)*nddof
   integer,parameter :: ncdof=nncm       *nddof


   integer          intg

   real(kind=DP_REAL_KIND) :: xel   (ndim  ,nnde  )

   real(kind=DP_REAL_KIND) :: dxdxl (ndim ,ndim )
                            !  Jaconbian matrix
   real(kind=DP_REAL_KIND) :: dxldx (ndim ,ndim )
   real(kind=DP_REAL_KIND) :: svol
   real(kind=DP_REAL_KIND) :: vol

   real(kind=DP_REAL_KIND) :: dxldx0(ndim ,ndim )
   real(kind=DP_REAL_KIND) :: svol0
   real(kind=DP_REAL_KIND) :: voln

   real(kind=DP_REAL_KIND) :: dndx (ndim ,nnde )
   real(kind=DP_REAL_KIND) :: dndxn(ndim ,nncm )

   real(kind=DP_REAL_KIND) :: bmatc(nstr ,nndof)
   real(kind=DP_REAL_KIND) :: bmatn(nstr ,ncdof)

   real(kind=DP_REAL_KIND) :: stfnn(ncdof,ncdof)
   real(kind=DP_REAL_KIND) :: stfcn(nndof,ncdof)

   real(kind=DP_REAL_KIND) :: fe   (nndof)

   integer,parameter :: nires=4

   real(kind=DP_REAL_KIND),dimension(ndim ,nires) :: xlspr

   character(len=13),parameter::eletyp='Quadrilateral'

contains

   subroutine setxlp

      implicit none

      xlspr=xlgp

   end subroutine setxlp

end module upelm

subroutine jac0

!  set element data

   use nedata
   use upelm
   use dconst

   implicit none

   real(kind=DP_REAL_KIND) :: dxdxl0(ndim ,ndim )

   integer          j
   integer          ix,kx

   dxdxl0=zero
   do kx=1,ndim
      do ix=1,ndim
         do j=1,nnde
            dxdxl0(kx,ix)=dxdxl0(kx,ix)+dndxl0(ix,j)*xel(kx,j)
         end do
      end do
   end do

   call sminv(ndim,dxdxl0,svol0,dxldx0)

end subroutine jac0

subroutine shcoef(jelm  )

!  set element data

   use nedata
   use upelm
   use dconst

   implicit none

   integer,intent(in ) :: jelm

   integer          j,jj
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
   vol =svol *wint(intg)
   voln=svol0*wint(intg)

   dndx =matmul(transpose(dxldx ),dndxl (:,:,intg))
   dndxn=matmul(transpose(dxldx0),dndxln(:,:,intg))

   bmatc =zero
   do j=1,nnde
      jj=nddof*(j-1)
      bmatc(1,jj+1)=dndx(1,j)
      bmatc(2,jj+2)=dndx(2,j)
      bmatc(3,jj+1)=dndx(2,j)
      bmatc(3,jj+2)=dndx(1,j)
   end do

   bmatn =zero
   do j=1,nncm
      jj=nddof*(j-1)
      bmatn(1,jj+1)=dndxn(1,j)
      bmatn(2,jj+2)=dndxn(2,j)
      bmatn(3,jj+1)=dndxn(2,j)
      bmatn(3,jj+2)=dndxn(1,j)
   end do

end subroutine shcoef

subroutine cestf(stfe)

!  calculate element stiffness matrix

   use upelm
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)

   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: dbmatc
   real(kind=DP_REAL_KIND),dimension(nstr,ncdof) :: dbmatn

   dbmatc=matmul(dmat,bmatc)
   dbmatn=matmul(dmat,bmatn)

   stfe =stfe +matmul(transpose(bmatc),dbmatc)*vol
   stfnn=stfnn+matmul(transpose(bmatn),dbmatn)*voln
   stfcn=stfcn+matmul(transpose(bmatc),dbmatn)*voln

end subroutine cestf

subroutine elmncm(stfe)

!  eliminate nonconforming modes

   use dconst
   use upelm

   implicit none

   real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)


   real(kind=DP_REAL_KIND),dimension(ncdof,nndof) :: sncp
   real(kind=DP_REAL_KIND),dimension(ncdof)       :: snv
   integer                ,dimension(ncdof)       :: ip

   real(kind=DP_REAL_KIND) :: ssum

   integer,parameter :: nns=nndof-1

   integer          i,j,k

   call pludec(ncdof,stfnn,snv,ip)
   do i=1,nndof
      snv(:)=stfcn(i,:)
      call plusol(ncdof,stfnn,snv,sncp(:,i),ip)
   end do

   do j=1,nndof
      do i=1,nndof
         ssum=zero
         do k=1,ncdof
            ssum=ssum+stfcn(i,k)*sncp(k,j)
         end do
         stfe(i,j)=stfe(i,j)-ssum
      end do
   end do

end subroutine elmncm

subroutine css(stress,jelm)

!  calculate strain and stress

   use upelm
   use cfmc
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),dimension(nstr,nires),intent(out) :: stress
   integer                                      ,intent(in ) :: jelm

   real(kind=DP_REAL_KIND),dimension(nndof )       :: uel
   real(kind=DP_REAL_KIND),dimension(ncdof )       :: ueln
   real(kind=DP_REAL_KIND),dimension(nstr  )       :: strain

   real(kind=DP_REAL_KIND),dimension(nndof,nndof)  :: stfe

   real(kind=DP_REAL_KIND),dimension(ncdof )       :: rv,wk
   integer                ,dimension(ncdof )       :: ip

   integer,parameter :: nns=nndof-1

   integer          i,j,k

   do i=1,nnde
      j=idelm(i,jelm)
      do k=1,ndim
         uel(nddof*(i-1)+k)=unode(k,j)
      end do
      xel(:,i)=xnode(:,j)
   end do

   stfe =zero
   stfnn=zero
   stfcn=zero

   call jac0

   do intg=1,nint
      call shcoef(jelm)
      call cestf(stfe)
   end do

   rv=zero
   do i=1,ncdof
      do j=1,nndof
         rv(i)=rv(i)-stfcn(j,i)*uel(j)
      end do
   end do

   call pludec(ncdof,stfnn,wk,ip)
   call plusol(ncdof,stfnn,rv,ueln,ip)

   do intg=1,nint
      call shcoef(jelm)
      strain=matmul(bmatc,uel)+matmul(bmatn,ueln)
      stress(:,intg)=matmul(dmatpr,strain)
   end do

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
