! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module solmod

   use kind_parameters
   use dconst
   use nedata

   integer                   :: ndof  ! Number of Total Degrees of Freedom
   integer                   :: nband ! Number of band width of matrix

   real(kind=DP_REAL_KIND),dimension(:,:),allocatable :: stf   
!                                             Stiffness matrix

   real(kind=DP_REAL_KIND),dimension(:),allocatable   :: unew
!                                  : Unknown vector for linear system
   real(kind=DP_REAL_KIND),dimension(:),allocatable   :: fvec  
!                                  : Right hand side vector for linear system

contains

   subroutine sdaloc

      implicit none

      print *,'ndof=',ndof,'  nband=',nband
      allocate(stf   (nband ,ndof  ))

      allocate(unew  (ndof  ))
      allocate(fvec  (ndof  ))
      unew(:)=zero
      fvec(:)=zero
      stf(:,:)=zero

   end subroutine sdaloc

   subroutine aseqcf(stfe  ,jelm  )

!     assemble system of equations for SLAP column format

      use nedata

      implicit none

      real(kind=DP_REAL_KIND),dimension(nndof ,nndof ),intent(in) :: stfe
      integer,                                         intent(in) :: jelm

      real(kind=DP_REAL_KIND) :: xsum

      integer          i1,j1,k1
      integer          ij1,jj1
      integer          i2,j2,k2
      integer          ij2,jj2
      integer          kb

!     assemble global matrix and vector

!     print *,'stfe'
!     write(*,'(1h ,8g12.3)') stfe

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
!                    print *,i1,j1,i2,j2
                     k1=idnd(i1,jj1)
!                    print *,k1,k2
                     ij1=(j1-1)*nddof+i1

                     if(k1.gt.0) then

                        kb=k2-k1+nband
                        if(kb.le.nband) then
                           stf(kb,k1)=stf(kb,k1)+stfe(ij1,ij2)
                        end if
                     else
                        xsum=xsum+stfe(ij2,ij1)*unode(i1,jj1)
                     end if
                  end do
               end do

               fvec(k2)=fvec(k2)-xsum
!              print *,'xsum',xsum

            end if

         end do
      end do

   end subroutine aseqcf

   subroutine stfsol(isgnd ,det   ,ierr  )

      implicit none

      real(kind=DP_REAL_KIND),intent(out)    :: det
      integer,intent(out)                    :: isgnd,ierr
      integer                                :: isw

      unew=fvec
      isw=1
!     print *,'stf core'
!     write(*,'(1h ,2g12.3)') stf(nband-1:nband,:)
      call mcsolv(ndof  ,nband ,unew  ,stf   ,det   ,&
     &            isgnd ,isw   )
      ierr=0
!     print *,'unew'
!     write(*,'(1h ,5g12.3)') unew

   end subroutine stfsol

end module solmod

subroutine mkmi

!     calculate band matrix

      use nedata
      use solmod

      implicit none

      integer          i,j,k
      integer          idof
      integer          nhbw
      integer          idmax,idmin

!     calculate band width

      nhbw=1
      do i=1,nelem
         idmax=1
         idmin=ndof
         do j=1,nnde
         do k=1,nddof
            idof=idnd(k,idelm(j,i))
            if(idof.ne.0) then
               idmax=max0(idmax,idof)
               idmin=min0(idmin,idof)
            end if
         end do
         end do
         nhbw=max0(idmax-idmin,nhbw)
      end do
      nband=nhbw+1

      call sdaloc

end subroutine mkmi

subroutine mcsolv(ms    ,nb    ,av    ,am    ,det   ,&
          &       isgnd ,isw   )

!     solve simmetric banded matrix
!     using modified choleski decomposition method

!     ms        :matrix size
!     nb        :band width
!     am(nb,ms) :coeficient matrix
!     av(nb)    :vector
!     det       :determinant(log)
!     isgnd     :sign of detreminant
!     isw       :switch
!                isw < 0 => cholesky decomposition only
!                isw = 0 => solve without decomposition
!                isw > 0 => solve

   implicit double precision (a-h,o-z)

   parameter ( one    = 1.0d0 )
   parameter ( zero   = 0.0d0 )
   parameter ( ueps   = 1.0d-20 )

   dimension av(ms)
   dimension am(nb,ms)

   if(isw.ne.0) then

!     choleski decomposition

      if(dabs(am(nb,1)).lt.ueps) goto 999

      do k=2,ms

         m1=max0(1,k-nb+1)
         sum=zero
         do j=m1,k-1
            amb=am(nb-k+j,k)/am(nb,j)
            sum=sum+am(nb-k+j,k)*amb
            am(nb-k+j,k)=amb
         end do
         am(nb,k)=am(nb,k)-sum
         if(dabs(am(nb,k)).lt.ueps) goto 999

         if(k.lt.ms) then
            m2=min0(k+nb-1,ms)
            do i=k+1,m2
               m3=max0(1,i-nb+1)
               sum=zero
               do j=m3,k-1
                  sum=sum+am(nb-i+j,i)*am(nb-k+j,k)
               end do
               am(nb-i+k,i)=am(nb-i+k,i)-sum
            end do
         endif

      end do
  260 continue

      call determ(am,det,isgnd,ms,nb)

      if(isw.lt.0) return

   end if

!     forward elimination

      do k=2,ms
         m1=max0(1,k-nb+1)
         sum=zero
         do j=m1,k-1
            sum=sum+am(nb-k+j,k)*av(j)
         end do
         av(k)=av(k)-sum
      end do

!     backward substitution

      av(ms)=av(ms)/am(nb,ms)
      do k=ms-1,1,-1
         m2=min0(k+nb-1,ms)
         sum=zero
         do j=k+1,m2
            sum=sum+am(nb-j+k,j)*av(j)
         end do
         av(k)=av(k)/am(nb,k)-sum
      end do

      return

!     matrix is singular.

  999 write(*,*) 'pivot is too small!'
      write(*,*) 'stop in mcsolv!'

contains

   subroutine determ(am,det,isgnd,ms,nb)
      implicit double precision (a-h,o-z)
      dimension am(nb,ms)
      det =0.d0
      isgnd=1
      do i=1,ms
         xx=am(nb,i)
         if(xx.lt.0.0) then
            isgnd=-isgnd
         end if
         det=det+dlog(dabs(xx))
      end do
   end subroutine determ

end subroutine mcsolv

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
