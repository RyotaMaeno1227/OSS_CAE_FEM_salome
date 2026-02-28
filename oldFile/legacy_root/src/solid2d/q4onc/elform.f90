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

   real(kind=DP_REAL_KIND) :: dndx (ndim ,nnde )
   real(kind=DP_REAL_KIND) :: dndxn(ndim ,nncm )

   real(kind=DP_REAL_KIND) :: bmat (nstr ,ntdof)

   real(kind=DP_REAL_KIND) :: stfet(ntdof,ntdof)

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

subroutine shcoef(jelm  )

!  set element data

   use nedata
   use upelm
   use dconst

   implicit none

   integer,intent(in ) :: jelm

   integer          j
   integer          jj
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

   dndx =matmul(transpose(dxldx),dndxl (:,:,intg))
   dndxn=matmul(transpose(dxldx),dndxln(:,:,intg))

   bmat =zero
   do j=1,nnde
      jj=nddof*(j-1)
      bmat(1,jj+1)=dndx(1,j)
      bmat(2,jj+2)=dndx(2,j)
      bmat(3,jj+1)=dndx(2,j)
      bmat(3,jj+2)=dndx(1,j)
   end do

   do j=1,nncm
      jj=nddof*(j-1)+nndof
      bmat(1,jj+1)=dndxn(1,j)
      bmat(2,jj+2)=dndxn(2,j)
      bmat(3,jj+1)=dndxn(2,j)
      bmat(3,jj+2)=dndxn(1,j)
   end do

end subroutine shcoef

subroutine cestf

!  calculate element stiffness matrix

   use upelm
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),dimension(nstr,ntdof) :: dbmat

   dbmat=matmul(dmat,bmat)

   stfet=stfet+matmul(transpose(bmat),dbmat)*vol

end subroutine cestf

subroutine elmncm(stfe)

!  eliminate nonconforming modes

   use dconst
   use upelm

   implicit none

   real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)

   real(kind=DP_REAL_KIND),dimension(ncdof,ncdof) :: stfnn

   real(kind=DP_REAL_KIND),dimension(ncdof,nndof) :: sncp
   integer                ,dimension(ncdof)       :: ip

   real(kind=DP_REAL_KIND) :: ssum

   integer,parameter :: nns=nndof-1

   integer          i,j,k

   do j=1,ncdof
      do i=1,ncdof
         stfnn(i,j)=stfet(i+nns,j+nns)
      end do
   end do

   call pludec(ncdof,stfnn,sncp(:,1),ip)
   do i=1,nndof
      call plusol(ncdof,stfnn,stfet(nndof+1,i),sncp(:,i),ip)
   end do

   do j=1,nndof
      do i=1,nndof
         ssum=zero
         do k=1,ncdof
            ssum=ssum+stfet(nndof+k,i)*sncp(k,j)
         end do
         stfe(i,j)=stfet(i,j)-ssum
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

   real(kind=DP_REAL_KIND),dimension(nstrss,nires ),intent(out) :: stress
   integer                                         ,intent(in ) :: jelm

   real(kind=DP_REAL_KIND),dimension(ntdof )       :: uel
   real(kind=DP_REAL_KIND),dimension(nstr  )       :: strain

   real(kind=DP_REAL_KIND),dimension(ncdof ,ncdof) :: stfnn
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

   stfet=zero

   do intg=1,nint
      call shcoef(jelm)
      call cestf
   end do

   do j=1,ncdof
      do i=1,ncdof
         stfnn(i,j)=stfet(i+nns,j+nns)
      end do
   end do

   rv=zero
   do i=1,ncdof
      do j=1,nndof
         rv(i)=rv(i)-stfet(nndof+i,j)*uel(j)
      end do
   end do

   call pludec(ncdof,stfnn,wk,ip)
   call plusol(ncdof,stfnn,rv,uel(nndof+1),ip)

   do intg=1,nint
      call shcoef(jelm)
      strain=matmul(bmat,uel)
      stress(:,intg)=matmul(dmatpr,strain)
   end do

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
