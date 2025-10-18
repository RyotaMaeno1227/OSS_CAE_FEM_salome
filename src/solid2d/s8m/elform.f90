! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module upelm

   use nedata
   use cfmc

   implicit none

   integer          intg

   real(kind=DP_REAL_KIND) :: xel   (ndim ,nnde  )

   real(kind=DP_REAL_KIND) :: dnmdxl (ndim ,nnde ,nint  )


   real(kind=DP_REAL_KIND) :: dxdxl (ndim ,ndim )
                            !  Jaconbian matrix
   real(kind=DP_REAL_KIND) :: dxldx (ndim ,ndim )
   real(kind=DP_REAL_KIND) :: svol
   real(kind=DP_REAL_KIND) :: vol

   real(kind=DP_REAL_KIND) :: dndx (ndim  ,nnde  )
   real(kind=DP_REAL_KIND),dimension(nstr,nndof) :: bmat

!  real(kind=DP_REAL_KIND) :: fve  (ndim  ,nelnd )

   real(kind=DP_REAL_KIND) :: fe   (nndof)

   integer,parameter :: nires=9

   real(kind=DP_REAL_KIND),dimension(ndim ,nires) :: xlspr

   character(len=13),parameter::eletyp='Quadrilateral'

contains

   subroutine setxlp

      use cfmc

      implicit none

      xlspr=xlgp

   end subroutine setxlp

end module upelm

subroutine modsh

!  calculate modified shape function

   use upelm
   use dconst

   implicit none

   real(kind=DP_REAL_KIND),dimension(4) :: djac
   real(kind=DP_REAL_KIND)              :: shcc,shcm

   integer,dimension(4,4),parameter :: msid &
  &=reshape( (/1,2,3,4, 2,3,4,1, 3,4,1,2, 4,1,2,3/) ,(/4,4/) )

   integer i,j,k,m
   integer ix

   do i=1,4
      j=msid(2,i)
      m=msid(4,i)

      djac(i)=(xel(1,j)-xel(1,i))*(xel(2,m)-xel(2,i)) &
     &       -(xel(2,j)-xel(2,i))*(xel(1,m)-xel(1,i))
   end do

   do i=1,4
      j=msid(2,i)
      k=msid(3,i)
      m=msid(4,i)

      shcc=d8*(djac(k)-djac(i))/(djac(i)+djac(k))
      shcm=d8*(djac(m)-djac(i))/(djac(i)+djac(k))
      do ix=1,ndim
         do intg=1,nint
            dnmdxl(ix,i  ,intg)=dndxl(ix,i  ,intg)-shcc*dndxl9(ix,intg)
            dnmdxl(ix,i+4,intg)=dndxl(ix,i+4,intg)+shcm*dndxl9(ix,intg)
         end do
      end do
   end do

end subroutine modsh

subroutine shcoef(jelm  )

!  set element data

   use nedata
   use upelm
   use cfmc
   use dconst

   implicit none

   integer,intent(in ) :: jelm

   integer          j,jj
   integer          ix,kx

   dxdxl=zero
   do kx=1,ndim
      do ix=1,ndim
         do j=1,nnde
            dxdxl(kx,ix)=dxdxl(kx,ix)+dnmdxl(ix,j,intg)*xel(kx,j)
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

   dndx=matmul(transpose(dxldx),dnmdxl(:,:,intg))

   bmat =zero
   do j=1,nnde
      jj=nddof*(j-1)
      bmat(1,jj+1)=dndx(1,j)
      bmat(2,jj+2)=dndx(2,j)
      bmat(3,jj+1)=dndx(2,j)
      bmat(3,jj+2)=dndx(1,j)
   end do

end subroutine shcoef

subroutine cestf (stfe  )

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

   real(kind=DP_REAL_KIND),dimension(nstrss,nires ),intent(out) :: stress
   integer                                         ,intent(in ) :: jelm

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
      strain=matmul(bmat,uel)
      stress(:,intg)=matmul(dmatpr,strain)
   end do

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
