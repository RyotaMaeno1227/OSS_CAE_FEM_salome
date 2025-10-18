! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module upelm

! for Discrete Kirchhoff Triangle Plate Element

   use nedata

   implicit none

   integer,parameter :: npstr=nstr*3
!  # of Parameters for Strain and Stress

   integer          intg

   real(kind=DP_REAL_KIND) :: xel   (ndim  ,nnde  )

   real(kind=DP_REAL_KIND) :: dc,ds
   real(kind=DP_REAL_KIND) :: dvol

   real(kind=DP_REAL_KIND),dimension(3,3,3,3) :: altm

   real(kind=DP_REAL_KIND),dimension(npstr,nndof) :: almat

!  real(kind=DP_REAL_KIND) :: fve   (ndim  ,nelnd )

   real(kind=DP_REAL_KIND) :: fe   (nndof)

   integer,parameter                              :: nires=1
   real(kind=DP_REAL_KIND),dimension(ndim ,nires) :: xlspr
   character(len=8),parameter                     :: eletyp='Triangle'

contains

   subroutine setxlp

      use dconst

      implicit none

      xlspr(1,1)=d3
      xlspr(2,1)=d3

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

   real(kind=DP_REAL_KIND) :: x2,x3,y3
   real(kind=DP_REAL_KIND) :: x12,x23,x31,y23,y31
   real(kind=DP_REAL_KIND) :: sl12,sl23,sl31
   real(kind=DP_REAL_KIND) :: p4,p5,p6,q4,q5,r4,r5,t4,t5

   real(kind=DP_REAL_KIND),dimension(3,3,3,3) :: al

   real(kind=DP_REAL_KIND) :: dx2,dy2,dx3,dy3
   real(kind=DP_REAL_KIND),dimension(3,3) :: ttmat

!   real(kind=DP_REAL_KIND) :: tv(nndof),rv(nndof)

   integer          i,j
   integer          ip,jp

   dx2=xel(1,2)-xel(1,1)
   dy2=xel(2,2)-xel(2,1)
   dx3=xel(1,3)-xel(1,1)
   dy3=xel(2,3)-xel(2,1)
   x2 =sqrt(dx2**2+dy2**2)
   dc=dx2/x2
   ds=dy2/x2

   x3= dx3*dc+dy3*ds
   y3=-dx3*ds+dy3*dc

   dvol=x2*y3

!  Transform matrix

   ttmat(1,1)=one
   ttmat(1,2)=zero
   ttmat(1,3)=zero
   ttmat(2,1)=zero
   ttmat(2,2)= dc
   ttmat(2,3)= ds
   ttmat(3,1)=zero
   ttmat(3,2)=-ds
   ttmat(3,3)= dc

!  Alpha matrix

   x12=  -x2
   x23=x2-x3
   x31=x3

   y23=  -y3
   y31=y3

   sl12=x12**2
   sl23=x23**2+y23**2
   sl31=x31**2+y31**2

   p4=-6.0d0*x23/sl23
   p5=-6.0d0*x3 /sl31
   p6=-6.0d0*x12/sl12

   t4=-6.0d0*y23/sl23
   t5=-6.0d0*y3 /sl31

   q4=three*x23*y23/sl23
   q5=three*x3 *y3 /sl31

   r4=three*(y23**2)/sl23
   r5=three*(y31**2)/sl31

   al(1,1,1,1)= y3*p6
   al(1,2,1,1)= zero
   al(1,3,1,1)=-four*y3
   al(2,1,1,1)=-y3*p6
   al(2,2,1,1)= zero
   al(2,3,1,1)= two*y3
   al(3,1,1,1)= y3*p5
   al(3,2,1,1)=-y3*q5
   al(3,3,1,1)= y3*(two-r5)
          
   al(1,1,1,2)=-y3*p6
   al(1,2,1,2)= zero
   al(1,3,1,2)=-two*y3
   al(2,1,1,2)= y3*p6
   al(2,2,1,2)= zero
   al(2,3,1,2)= four*y3
   al(3,1,1,2)= y3*p4
   al(3,2,1,2)= y3*q4
   al(3,3,1,2)= y3*(r4-two)

   al(1,1,1,3)= zero
   al(1,2,1,3)= zero
   al(1,3,1,3)= zero
   al(2,1,1,3)= zero
   al(2,2,1,3)= zero
   al(2,3,1,3)= zero
   al(3,1,1,3)=-y3*(p4+p5)
   al(3,2,1,3)= y3*(q4-q5)
   al(3,3,1,3)= y3*(r4-r5)
          
   al(1,1,2,1)=-x2*t5
   al(1,2,2,1)= x23+x2*r5
   al(1,3,2,1)=-x2*q5
   al(2,1,2,1)= zero
   al(2,2,2,1)= x23
   al(2,3,2,1)= zero
   al(3,1,2,1)= x23*t5
   al(3,2,2,1)= x23*(one-r5)
   al(3,3,2,1)= x23*q5
             
   al(1,1,2,2)= zero
   al(1,2,2,2)= x3
   al(1,3,2,2)= zero
   al(2,1,2,2)= x2*t4
   al(2,2,2,2)= x3+x2*r4
   al(2,3,2,2)=-x2*q4
   al(3,1,2,2)=-x3*t4
   al(3,2,2,2)= x3*(one-r4)
   al(3,3,2,2)= x3*q4
             
   al(1,1,2,3)= x2*t5
   al(1,2,2,3)= x2*(r5-one)
   al(1,3,2,3)=-x2*q5
   al(2,1,2,3)=-x2*t4
   al(2,2,2,3)= x2*(r4-one)
   al(2,3,2,3)=-x2*q4
   al(3,1,2,3)=-x23*t5+x3*t4
   al(3,2,2,3)=-x23*r5-x3*r4-x2
   al(3,3,2,3)= x3*q4+x23*q5
             
   al(1,1,3,1)=-x3*p6-x2*p5
   al(1,2,3,1)= x2*q5+y3
   al(1,3,3,1)=-four*x23+x2*r5
   al(2,1,3,1)=-x23*p6
   al(2,2,3,1)= y3
   al(2,3,3,1)= two*x23
   al(3,1,3,1)= x23*p5+y3*t5
   al(3,2,3,1)=-x23*q5+(one-r5)*y3
   al(3,3,3,1)= (two-r5)*x23+y3*q5
             
   al(1,1,3,2)= x3*p6
   al(1,2,3,2)=-y3
   al(1,3,3,2)= two*x3
   al(2,1,3,2)= x23*p6+x2*p4
   al(2,2,3,2)=-y3+x2*q4
   al(2,3,3,2)=-four*x3+x2*r4
   al(3,1,3,2)=-x3*p4+y3*t4
   al(3,2,3,2)= (r4-one)*y3-x3*q4
   al(3,3,3,2)= (two-r4)*x3-y3*q4
             
   al(1,1,3,3)= x2*p5
   al(1,2,3,3)= x2*q5
   al(1,3,3,3)= (r5-two)*x2
   al(2,1,3,3)=-x2*p4
   al(2,2,3,3)= x2*q4
   al(2,3,3,3)= (r4-two)*x2
   al(3,1,3,3)=-x23*p5+x3*p4-(t4+t5)*y3
   al(3,2,3,3)=-x23*q5-x3*q4+(r4-r5)*y3
   al(3,3,3,3)=-x23*r5-x3*r4+four*x2+(q5-q4)*y3

   do jp=1,3
      do ip=1,3
         do j=1,3
            do i=1,3
               altm(i,j,ip,jp)=al(i,1,ip,jp)*ttmat(1,j) &
                             &+al(i,2,ip,jp)*ttmat(2,j) &
                             &+al(i,3,ip,jp)*ttmat(3,j)
            end do
         end do
      end do
   end do

   do j=1,nddof
      do i=1,nstr
         almat(i  ,j  )=altm(i,j,1,1)
         almat(i+3,j  )=altm(i,j,2,1)
         almat(i+6,j  )=altm(i,j,3,1)
         almat(i  ,j+3)=altm(i,j,1,2)
         almat(i+3,j+3)=altm(i,j,2,2)
         almat(i+6,j+3)=altm(i,j,3,2)
         almat(i  ,j+6)=altm(i,j,1,3)
         almat(i+3,j+6)=altm(i,j,2,3)
         almat(i+6,j+6)=altm(i,j,3,3)
      end do
   end do

   if(dvol.le.0) then
      print *,"J<0",jelm
      print *,'dvol,xel' 
      write(*,'(1h ,g12.3,a,3g12.3)') dvol,":",xel
   end if

end subroutine shcoef

subroutine cestf(stfe)

!  calculate element stiffness matrix

   use upelm
   use dconst
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),intent(inout) :: stfe (nndof,nndof)

   real(kind=DP_REAL_KIND),dimension(nndof,nndof) :: qmat

   real(kind=DP_REAL_KIND) :: e1,e2,e3,e4
   real(kind=DP_REAL_KIND) :: coef


   real(kind=DP_REAL_KIND),dimension(3,3),parameter :: rmat       &
  &=reshape( (/two,one,one,one,two,one,one,one,two/), (/3,3/) )

   e1=dmat(1,1)
   e2=dmat(1,2)
   e3=dmat(2,2)
   e4=dmat(3,3)

   qmat(1:3,1:3)=matmul(transpose( e1*altm(:,:,1,1)       &
                &                 +e2*altm(:,:,2,1) ),rmat)
   qmat(1:3,4:6)=matmul(transpose( e2*altm(:,:,1,1)       &
                &                 +e3*altm(:,:,2,1) ),rmat)
   qmat(1:3,7:9)=matmul(transpose( e4*altm(:,:,3,1) ),rmat)
   qmat(4:6,1:3)=matmul(transpose( e1*altm(:,:,1,2)       &
                &                 +e2*altm(:,:,2,2) ),rmat)
   qmat(4:6,4:6)=matmul(transpose( e2*altm(:,:,1,2)       &
                &                 +e3*altm(:,:,2,2) ),rmat)
   qmat(4:6,7:9)=matmul(transpose( e4*altm(:,:,3,2) ),rmat)
   qmat(7:9,1:3)=matmul(transpose( e1*altm(:,:,1,3)       &
                &                 +e2*altm(:,:,2,3) ),rmat)
   qmat(7:9,4:6)=matmul(transpose( e2*altm(:,:,1,3)       &
                &                 +e3*altm(:,:,2,3) ),rmat)
   qmat(7:9,7:9)=matmul(transpose( e4*altm(:,:,3,3) ),rmat)

   coef=one/(24.d0*dvol)

   stfe=matmul(qmat,almat)*coef

end subroutine cestf

subroutine css(stress,jelm)

!  calculate strain and stress

   use upelm
   use cfmc
   use matmod

   implicit none

   real(kind=DP_REAL_KIND),dimension(nstrss,nires),intent(out) :: stress
   integer                                        ,intent(in ) :: jelm

   real(kind=DP_REAL_KIND),dimension(nndof)       :: uel
   real(kind=DP_REAL_KIND),dimension(nstr )       :: strain
   real(kind=DP_REAL_KIND),dimension(npstr)       :: pstrn
   real(kind=DP_REAL_KIND),dimension(ndim ,ndim ) :: trt,stl,trstl

   real(kind=DP_REAL_KIND) :: coef

   integer          i,j,k,ii

   do i=1,nnde
      j=idelm(i,jelm)
      do k=1,nddof
         uel(nddof*(i-1)+k)=unode(k,j)
      end do
      xel(:,i)=xnode(:,j)
   end do

   call shcoef(jelm)

   coef=one/(three*dvol)

   pstrn=matmul(almat,uel)
   do i=1,nstr
      ii=i-1
      strain(i)=(pstrn(ii*3+1)+pstrn(ii*3+2)+pstrn(ii*3+3))*coef
   end do
   stress(:,1)=matmul(dmat,strain)

   stl(1,1)=stress(1,1)
   stl(1,2)=stress(3,1)
   stl(2,1)=stress(3,1)
   stl(2,2)=stress(2,1)

   trt(1,1)= dc
   trt(1,2)=-ds
   trt(2,1)= ds
   trt(2,2)= dc

   trstl=matmul(trt,stl)
   stress(1,1)=trstl(1,1)*trt(1,1)+trstl(1,2)*trt(1,2)
   stress(2,1)=trstl(2,1)*trt(2,1)+trstl(2,2)*trt(2,2)
   stress(3,1)=trstl(1,1)*trt(2,1)+trstl(1,2)*trt(2,2)

end subroutine css

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
