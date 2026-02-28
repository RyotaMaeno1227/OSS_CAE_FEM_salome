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

   real(kind=DP_REAL_KIND) :: xel   (ndim ,nnde  )

   real(kind=DP_REAL_KIND) :: dxdxl (ndim ,ndim )
                            !  Jaconbian matrix
   real(kind=DP_REAL_KIND) :: dxldx (ndim ,ndim )
   real(kind=DP_REAL_KIND) :: svol
   real(kind=DP_REAL_KIND) :: vol

   real(kind=DP_REAL_KIND) :: dndx (ndim ,nnde )

   real(kind=DP_REAL_KIND) :: dndxn(ndim ,nncm )
   real(kind=DP_REAL_KIND) :: dndxln(ndim ,nncm ,nint )

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

subroutine ncsh

!  calculate nonconforming shape function
!  based on Ishii and Kikuchi formulation

   use upelm
   use dconst

   implicit none
   real(kind=DP_REAL_KIND) :: dnmdxl(ndim ,nmnd ,nint)

   real(kind=DP_REAL_KIND),dimension(4)   :: djac
   real(kind=DP_REAL_KIND)                :: shcm

   real(kind=DP_REAL_KIND),dimension(4,2) :: ac
   real(kind=DP_REAL_KIND)                :: ad
   real(kind=DP_REAL_KIND)                :: ssum

   integer,dimension(4,4),parameter :: msid &
  &=reshape( (/1,2,3,4, 2,3,4,1, 3,4,1,2, 4,1,2,3/) ,(/4,4/) )

   integer i,j,k,m
   integer ix

   do i=1,nmnd
      j=msid(2,i)
      m=msid(4,i)

      djac(i)=(xel(1,j)-xel(1,i))*(xel(2,m)-xel(2,i)) &
     &       -(xel(2,j)-xel(2,i))*(xel(1,m)-xel(1,i))
   end do

   ad=two/(  (xel(1,2)+xel(1,3)-xel(1,1)-xel(1,4))&
  &         *(xel(2,3)+xel(2,4)-xel(2,1)-xel(2,2))&
  &        + (xel(2,2)+xel(2,3)-xel(2,1)-xel(2,4))&
  &         *(xel(1,3)+xel(1,4)-xel(1,1)-xel(1,2)))

   ac(1,1)=ad*( (xel(1,3)-xel(1,4))                  &
  &            *(xel(2,3)+xel(2,4)-xel(2,1)-xel(2,2))&
  &            -(xel(2,3)-xel(2,4))                  &
  &            *(xel(1,3)+xel(1,4)-xel(1,1)-xel(1,2)))

   ac(2,1)=ad*(-(xel(1,1)-xel(1,2))                  &
  &            *(xel(2,2)+xel(2,3)-xel(2,1)-xel(2,4))&
  &            +(xel(2,1)-xel(2,2))                  &
  &            *(xel(1,2)+xel(1,3)-xel(1,1)-xel(1,4)))

   ac(3,1)=ad*( (xel(1,2)-xel(1,1))                  &
  &            *(xel(2,3)+xel(2,4)-xel(2,1)-xel(2,2))&
  &            -(xel(2,2)-xel(2,1))                  &
  &            *(xel(1,3)+xel(1,4)-xel(1,1)-xel(1,2)))

   ac(4,1)=ad*(-(xel(1,2)-xel(1,1))                  &
  &            *(xel(2,2)+xel(2,3)-xel(2,1)-xel(2,4))&
  &            +(xel(2,2)-xel(2,1))                  &
  &            *(xel(1,2)+xel(1,3)-xel(1,1)-xel(1,4)))

   ac(1,2)=ad*( (xel(1,2)-xel(1,3))                  &
  &            *(xel(2,3)+xel(2,4)-xel(2,1)-xel(2,2))&
  &            -(xel(2,2)-xel(2,3))                  &
  &            *(xel(1,3)+xel(1,4)-xel(1,1)-xel(1,2)))

   ac(2,2)=ad*(-(xel(1,4)-xel(1,1))                  &
  &            *(xel(2,2)+xel(2,3)-xel(2,1)-xel(2,4))&
  &            +(xel(2,4)-xel(2,1))                  &
  &            *(xel(1,2)+xel(1,3)-xel(1,1)-xel(1,4)))

   ac(3,2)=ad*( (xel(1,3)-xel(1,2))                  &
  &            *(xel(2,3)+xel(2,4)-xel(2,1)-xel(2,2))&
  &            -(xel(2,3)-xel(2,2))                  &
  &            *(xel(1,3)+xel(1,4)-xel(1,1)-xel(1,2)))

   ac(4,2)=ad*(-(xel(1,3)-xel(1,2))                  &
  &            *(xel(2,2)+xel(2,3)-xel(2,1)-xel(2,4))&
  &            +(xel(2,3)-xel(2,2))                  &
  &            *(xel(1,2)+xel(1,3)-xel(1,1)-xel(1,4)))

   do i=1,nmnd
      j=msid(2,i)
      k=msid(3,i)
      m=msid(4,i)

      shcm=d2+d8*(djac(m)-djac(i))/(djac(i)+djac(k))

      do ix=1,ndim
         do intg=1,nint
            dnmdxl(ix,i,intg)=dndxlm(ix,i,intg)+shcm*dndxl9(ix,intg)
         end do
      end do
   end do

   do i=1,nncm
      do ix=1,ndim
         do intg=1,nint
            ssum=zero
            do k=1,nmnd
               ssum=ssum+ac(k,i)*dnmdxl(ix,k,intg)
            end do
            dndxln(ix,i,intg)=ssum
         end do
      end do
   end do

end subroutine ncsh

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

   real(kind=DP_REAL_KIND),dimension(nstr,nires),intent(out) :: stress
   integer                                      ,intent(in ) :: jelm

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

   call ncsh

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
   do j=1,ncdof
      do i=1,nndof
         rv(j)=rv(j)-stfet(nndof+j,i)*uel(i)
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
