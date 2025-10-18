! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module solmod

   use kind_parameters
   use dconst
   use nedata

   integer                              :: ndof
!                                       Number of Total Degrees of Freedom
   integer                              :: nelt
!                                       Number of nonzero entries for row

   integer,allocatable,dimension(:)     :: ia     ! Index of matrix
   integer,allocatable,dimension(:)     :: ja     ! Index of matrix
   integer,allocatable,dimension(:,:,:) :: idstf
                                         ! Index of element stiffness matrix

   real(kind=DP_REAL_KIND),dimension(:),allocatable :: stf   
!                                             Stiffness matrix

   real(kind=DP_REAL_KIND),dimension(:),allocatable   :: unew
!                                  : Unknown vector for linear system
   real(kind=DP_REAL_KIND),dimension(:),allocatable   :: fvec  
!                                  : Right hand side vector for linear system

contains

   subroutine sdaloc

      implicit none

      allocate(stf   (nelt  ))
      allocate(ia    (nelt  ))
      allocate(ja    (ndof+1))
      allocate(idstf (nndof ,nndof ,nelem  ))

      allocate(unew  (ndof  ))
      allocate(fvec  (ndof  ))
      unew(:)=zero
      fvec(:)=zero
      stf(:)=zero

   end subroutine sdaloc

   subroutine aseqcf(stfe  ,jelm  )

!     assemble system of equations for SLAP column format

      use nedata

      implicit none

      real(kind=DP_REAL_KIND),dimension(nndof ,nndof ),intent(in) :: stfe
      integer,                                         intent(in) :: jelm

      real(kind=DP_REAL_KIND) :: xsum

      integer          i1,j1,k1
      integer          ij1,jj1,ik1
      integer          i2,j2,k2
      integer          ij2,jj2,ik2
      integer          ij
      integer          icol

!     assemble global matrix and vector

      do j2=1,nnde
         jj2=idelm(j2,jelm)
         do i2=1,nddof
            k2=idnd(i2,jj2)

            if(k2.gt.0) then

               ij2 =(j2-1)*nddof+i2
               xsum=0.0d0
               do j1=1,nnde
                  jj1=idelm(j1,jelm)
                  do i1=1,nddof
                     k1=idnd(i1,jj1)
                     ij1=(j1-1)*nddof+i1
                     if(k1.ge.k2) then
                        ij=idstf(ij2,ij1,jelm)
                        stf(ij)=stf(ij)+stfe(ij2,ij1)
                     else if(k1.eq.0) then
                        if(i1.le.ndim) then
                           xsum=xsum+stfe(ij2,ij1)*unode(i1,jj1)
!                          print *,'du',i1,jj1,stfe(ij2,ij1),unode(i1,jj1)
                        end if
                     end if
                  end do
               end do

               fvec(k2)=fvec(k2)-xsum
!              print *,'xsum',xsum

            end if

         end do
      end do

   end subroutine aseqcf

   subroutine stfsol(iter  ,err   ,ierr  )

      implicit none

      integer,parameter                      :: iunit=6
      integer,parameter                      :: itmax=50000
      real(kind=DP_REAL_KIND),parameter      :: tol=1.0d-12

      real(kind=DP_REAL_KIND),intent(out)    :: err
      integer,intent(out)                    :: iter,ierr

!     print *,'fvec'
!     write(*,'(5g12.3)') fvec 
      call cgsol (ndof  ,nelt  ,stf   ,fvec  ,unew  , &
                & ia    ,ja    ,itmax ,tol   ,iter  , &
                & err   ,ierr  ,iunit )
 
   end subroutine stfsol

end module solmod

subroutine mkmi

!  make matrix index for row format

   use solmod

   implicit none

   integer          i1,j1,k1,ij1,ik1,ijk1
   integer          i2,j2,k2,ij2,ik2,ijk2
   integer          jj1,jj2
   integer          jelm
   integer          jce
   integer          i,j
   integer          icemmx
   integer          ixsum
   integer          nc

   integer,dimension(ndof) :: icem

   type mtlist
      integer :: irow
      type(mtlist), pointer::next
   end type

   type(mtlist),pointer,dimension(:)::mtpr
   type(mtlist),pointer::ptr1,ptr2

!  Initialize Pointers

   allocate(mtpr(ndof))
   do i=1,ndof
      mtpr(i)%irow=i
      nullify (mtpr(i)%next)
      icem(i)=1
   end do

!  Make Matrix List

   do jelm=1,nelem
      do j1=1,nnde
         jj1=idelm(j1,jelm)
         do i1=1,nddof
            ijk1=idnd(i1,jj1)
            if(ijk1.gt.0) then
               do j2=1,nnde
                  jj2=idelm(j2,jelm)
                  do i2=1,nddof
                     ijk2=idnd(i2,jj2)
                     if(ijk2.ge.ijk1) then
!                       call ckmkml(ijk2,icem(ijk1),mtpr(ijk1))
                        call ckmkml(ijk2,icem(ijk1),ijk1)
                     end if
                  end do
               end do
            endif
         end do
      end do
   end do


!  Calulate Number of Nonzero entries

   nelt=0
   do i=1,ndof
      nelt=nelt+icem(i)
   end do

!  print *,'icem'
!  print *,icem
   print *,'nelt=',nelt
   call sdaloc
!  print *,'end sdaloc'
   call mkidstf
!  print *,'end mkidstf'
!  print *,'ja'
!  print *,ja

   idstf=0
   do jelm=1,nelem

      do j1=1,nnde
         jj1=idelm(j1,jelm)
         do i1=1,nddof
            ijk1=idnd(i1,jj1)
            if(ijk1.gt.0) then
               ij1=(j1-1)*nddof+i1
               do j2=1,nnde
                  jj2=idelm(j2,jelm)
                  do i2=1,nddof
                     ijk2=idnd(i2,jj2)
                     if(ijk2.ge.ijk1) then
                        ij2=(j2-1)*nddof+i2
                        idstf(ij1,ij2,jelm)=idirow(ijk1,ijk2)
                     end if
                  end do
               end do
            endif
         end do
      end do
   end do

contains

   subroutine ckmkml(jrow,jcem,idof)
!  subroutine ckmkml(jrow,jcem,ptrr)

!     Check and Mark Matrix List

      implicit none
!     type(mtlist),pointer::ptr1,ptr2

      integer     ,intent(in)    :: jrow
      integer     ,intent(inout) :: jcem
      integer     ,intent(in)    :: idof
!     type(mtlist),pointer,intent(inout) :: ptrr

      integer     :: icount

      ptr1=>mtpr(idof)
      ptr2=>ptr1%next

      icount=1
      do
         if(jrow.eq.ptr1%irow) then
            icount=0
            exit
         end if
         if(associated(ptr2)) then
            ptr1=>ptr2
            ptr2=>ptr1%next
            icount=icount+1
         else
            exit
         end if
      end do

      if(icount>0) then
         allocate(ptr2)
         ptr2%irow=jrow
         nullify(ptr2%next)
         ptr1%next=>ptr2
         jcem=icount+1
      end if

   end subroutine ckmkml

   subroutine mkidstf

      integer::i,j,ic

      ic=1
      do i=1,ndof
         ja(i )=ic
         ptr1=>mtpr(i)
         ptr2=>ptr1%next
         do
            ia(ic)=ptr1%irow
            ic=ic+1
            if(associated(ptr2)) then
               ptr1=>ptr2
               ptr2=>ptr1%next
            else
               exit
            end if
         end do
      end do

      ja(ndof+1)=ic

   end subroutine mkidstf

   integer function idirow(idof  ,jdof )

      integer idof
      integer jdof
      integer k

      idirow=0
      do k=ja(idof),ja(idof+1)-1
         if(ia(k).eq.jdof) then
            idirow=k
            exit
         end if
      end do
      if(idirow.eq.0) then
         write(*,*) 'error',idof,jdof
      end if

   end function idirow

end subroutine mkmi

subroutine cgsol (n     ,nelt  ,a     ,b     ,x     , &
                & ia    ,ja    ,itmax ,tol   ,iter  , &
                & err   ,ierr  ,iunit )

!  Condugate Gradient Iterative solver
!  (with Diagonal Scaling Preconditioner)

   use kind_parameters

   implicit none

   integer,                intent(in ) :: n,nelt

   real(kind=DP_REAL_KIND),intent(in ) :: a     (nelt  )
   real(kind=DP_REAL_KIND),intent(in ) :: b     (n     )
   real(kind=DP_REAL_KIND),intent(out) :: x     (n     )
   integer,                intent(in ) :: ia    (nelt  )
   integer,                intent(in ) :: ja    (nelt  )


   integer,                intent(in ) :: itmax

   real(kind=DP_REAL_KIND),intent(in ) :: tol
   real(kind=DP_REAL_KIND),intent(out) :: err
   integer                ,intent(out) :: iter,ierr
   integer                ,intent(in ) :: iunit

   real(kind=DP_REAL_KIND) :: rvnrm ,rvnrmo,pven  ,fnrm
   real(kind=DP_REAL_KIND) :: alpha ,beta
   real(kind=DP_REAL_KIND) :: rzipo ,rzipn
   integer                 :: i
   integer,parameter       :: ncres=100

   real(kind=DP_REAL_KIND),allocatable,dimension(:) :: qv,rv,pv,zv
   real(kind=DP_REAL_KIND),allocatable,dimension(:) :: cmi

   allocate(qv (n))
   allocate(rv (n))
   allocate(pv (n))
   allocate(zv (n))
   allocate(cmi(n))

!  do i=1,n
!     cmi(i)=1.0d0/a(ja(i))
!     cmi(i)=1.0d0/sqrt(a(ja(i)))
!     cmi(i)=a(ja(i))
!  end do

!  initial residual

   fnrm=sum(b**2)

   call pprdmv(qv    ,x     )

   rv=b-qv

   rvnrm=sum(rv**2)

   if(iunit.ge.0) write(iunit,6000) 0,rvnrm
   if(rvnrm.le.tol*fnrm) then
      err =rvnrm
      ierr=0
      return
   end if
!
!  start iteration
!
   ierr=1
   do iter=1,itmax

!     zv(:)=cmi(:)*rv(:)
      zv(:)=rv(:)

      rzipn=dot_product(rv,zv)

      if(iter.eq.1) then
         pv=rv
      else
         beta=rzipn/rzipo
         pv=zv+beta*pv
      end if

      call pprdmv(qv   ,pv    )

      pven =dot_product(qv,pv)
      alpha =rzipn/pven

      x(:)=x(:)+alpha*pv(:)

      if(mod(iter,ncres).eq.0) then
         rvnrmo=sum(rv**2)
         call pprdmv(rv    ,x     )
         rv=b-rv
         rvnrm=sum(rv**2)
         write(*,*) 'res. norm old/new',rvnrmo,rvnrm
      else
         rv(:)=rv(:)-alpha*qv(:)
      end if

      rzipo=rzipn

      rvnrm=sum(rv**2)
      if(iunit.eq.0) then
         write(iunit,6000) iter,rvnrm
      else if(iunit.gt.0) then
         if(mod(iter,100).eq.0) write(iunit,6000) iter,rvnrm
      end if

      if(rvnrm.le.tol*fnrm) then
         ierr=0
         exit
      end if

   end do

   err =rvnrm/fnrm

 6000 format(" iter=",i5,"   residual=",g12.3)

contains

   subroutine pprdmv (y     ,x     )

!     Product preconditioned matrix and vector
!     y=a*x

      implicit none

      real(kind=DP_REAL_KIND),parameter   :: zero=0.0d0

      real(kind=DP_REAL_KIND),intent(in)  :: x     (n)
      real(kind=DP_REAL_KIND),intent(out) :: y     (n)

      integer          i,icol,ibgn,iend

      do i=1,n
         y(i)=zero
      end do

      do icol=1,n
         ibgn=ja(icol)
         iend=ja(icol+1)-1
         do i=ibgn,iend
            y(ia(i))=y(ia(i))+a(i)*x(icol)
         end do
         do i=ibgn+1,iend
            y(icol)=y(icol)+a(i)*x(ia(i))
         end do
      end do

   end subroutine pprdmv

end subroutine cgsol

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
