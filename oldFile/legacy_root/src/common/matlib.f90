! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

subroutine sminv(n,amat,detj,ain)

!  calculate small matrix(2x2, 3x3) inverse

   implicit none

   integer, parameter  :: DP_REAL_KIND = kind (0.0d0)
   integer, intent(in) :: n

   real(kind=DP_REAL_KIND),dimension(n,n),intent(in)  :: amat
   real(kind=DP_REAL_KIND),                     intent(out) :: detj
   real(kind=DP_REAL_KIND),dimension(n,n),intent(out) :: ain

   real(kind=DP_REAL_KIND) :: dvdetj

   if(n.eq.2) then

      detj=amat(1,1)*amat(2,2)-amat(1,2)*amat(2,1)
      dvdetj=1.0d0/detj

      ain(1,1)= amat(2,2)*dvdetj
      ain(1,2)=-amat(1,2)*dvdetj
      ain(2,1)=-amat(2,1)*dvdetj
      ain(2,2)= amat(1,1)*dvdetj

   else if(n.eq.3) then

      detj=amat(1,1)*amat(2,2)*amat(3,3) &
        & +amat(1,2)*amat(2,3)*amat(3,1) &
        & +amat(1,3)*amat(2,1)*amat(3,2) &
        & -amat(1,1)*amat(2,3)*amat(3,2) &
        & -amat(1,2)*amat(2,1)*amat(3,3) &
        & -amat(1,3)*amat(2,2)*amat(3,1)
      dvdetj=1.0d0/detj
  
      ain(1,1)= (amat(2,2)*amat(3,3)-amat(2,3)*amat(3,2) )*dvdetj
      ain(1,2)=-(amat(1,2)*amat(3,3)-amat(1,3)*amat(3,2) )*dvdetj
      ain(1,3)= (amat(1,2)*amat(2,3)-amat(1,3)*amat(2,2) )*dvdetj
      ain(2,1)=-(amat(2,1)*amat(3,3)-amat(2,3)*amat(3,1) )*dvdetj
      ain(2,2)= (amat(1,1)*amat(3,3)-amat(1,3)*amat(3,1) )*dvdetj
      ain(2,3)=-(amat(1,1)*amat(2,3)-amat(1,3)*amat(2,1) )*dvdetj
      ain(3,1)= (amat(2,1)*amat(3,2)-amat(2,2)*amat(3,1) )*dvdetj
      ain(3,2)=-(amat(1,1)*amat(3,2)-amat(1,2)*amat(3,1) )*dvdetj
      ain(3,3)= (amat(1,1)*amat(2,2)-amat(1,2)*amat(2,1) )*dvdetj

   end if

end subroutine sminv

subroutine pludec(n,a,x,ip)

!  LU decomposition with partial pivoting

   implicit none

   integer, parameter :: DP_REAL_KIND = kind (0.0d0)

   real(kind=DP_REAL_KIND), parameter :: one   =1.d0
   real(kind=DP_REAL_KIND), parameter :: eps   =1.d-20

   integer,intent(in) :: n

   real(kind=DP_REAL_KIND),dimension(n,n),intent(inout) :: a
   real(kind=DP_REAL_KIND),dimension(n)  ,intent(inout) :: x

   integer,dimension(n),intent(inout) :: ip

   real(kind=DP_REAL_KIND) :: al

   integer :: i,j,k,l
   integer :: lv

   do k=1,n
      ip(k)=k
   end do

   do k=1,n-1
      l=k
      al=abs(a(ip(l),k))
      do i=k+1,n
         if(abs(a(ip(i),k)).gt.al) then
            l=i
            al=abs(a(ip(l),k))
         end if
      end do
             
      if(l.ne.k) then
         lv=ip(k)
         ip(k)=ip(l)
         ip(l)=lv
      end if

      if(abs(a(ip(k),k)).lt.eps) then
         print *,'pludec zero pivot i=',k
         return
      end if

      a(ip(k),k)=one/a(ip(k),k)
      do i=k+1,n
         a(ip(i),k)=a(ip(i),k)*a(ip(k),k)
         do j=k+1,n
            x(j)=a(ip(i),j)-a(ip(i),k)*a(ip(k),j)
         end do
         do j=k+1,n
            a(ip(i),j)=x(j)
         end do
      end do

   end do
   if(abs(a(ip(n),n)).lt.eps) then
      print *,'pludec zero pivot i=',n
      return
   end if

   a(ip(n),n)=one/a(ip(n),n)

end subroutine pludec

subroutine plusol(n,a,b,x,ip)

!  Solve linear system using the matrix decomposed by pludec

   implicit none

   integer, parameter :: DP_REAL_KIND = kind (0.0d0)

   integer,intent(in) :: n

   real(kind=DP_REAL_KIND),dimension(n,n),intent(in)  :: a
   real(kind=DP_REAL_KIND),dimension(n)  ,intent(in)  :: b
   real(kind=DP_REAL_KIND),dimension(n)  ,intent(out) :: x

   integer,dimension(n),intent(in)                    :: ip

   real(kind=DP_REAL_KIND) :: xsum

   integer :: i,j
   
   x(1)=b(ip(1))
   do i=2,n
      xsum=b(ip(i))
      do j=1,i-1
         xsum=xsum-a(ip(i),j)*x(j)
      end do
      x(i)=xsum
   end do
   
   x(n)=x(n)*a(ip(n),n)
   do i=n-1,1,-1
      xsum=x(i)
      do j=i+1,n
         xsum=xsum-a(ip(i),j)*x(j)
      end do
      x(i)=xsum*a(ip(i),i)
   end do

end subroutine plusol

subroutine matinv(n,a,ainv,wk,ip)

   implicit none

   integer, parameter :: DP_REAL_KIND = kind (0.0d0)

   real(kind=DP_REAL_KIND), parameter :: zero  =0.d0
   real(kind=DP_REAL_KIND), parameter :: one   =1.d0
 
   integer,intent(in) :: n

   real(kind=DP_REAL_KIND),dimension(n,n),intent(inout) :: a
   real(kind=DP_REAL_KIND),dimension(n,n),intent(out)   :: ainv
   real(kind=DP_REAL_KIND),dimension(n)  ,intent(inout) :: wk
   integer,                dimension(n)  ,intent(inout) :: ip

   integer i

   call pludec(n,a,wk,ip)

   do i=1,n
     wk=zero
     wk(i)=one
     call plusol(n,a,wk,ainv(:,i),ip)
   end do

end subroutine matinv

subroutine crprd(v1,v2,ev)

!  cross product of 3D vectors

   implicit none

   integer,parameter :: DP_REAL_KIND = kind (0.0d0)

   real(kind=DP_REAL_KIND),dimension(3),intent(in) :: v1,v2
   real(kind=DP_REAL_KIND),dimension(3)            :: ev

   ev(1)=v1(2)*v2(3)-v1(3)*v2(2)
   ev(2)=v1(3)*v2(1)-v1(1)*v2(3)
   ev(3)=v1(1)*v2(2)-v1(2)*v2(1)

end subroutine crprd

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
