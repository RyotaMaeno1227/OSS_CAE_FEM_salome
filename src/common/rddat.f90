! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

subroutine rddat

!     read data

      use ionumb
      use nedata
      use solmod
      use trv
      use matmod
      use bpload

      implicit none

!     real(kind=DP_REAL_KIND),parameter :: vlval =1.0d5

      real(kind=DP_REAL_KIND) :: ful   (nddof )

      integer          ndel  (nnde  )
      integer          lbc   (nddof )

      integer          i,j,k
      integer          nn,ne
      integer          idof

      character(len=80) lline

      read(ir,*)
      read(ir,'(a)') title
      read(ir,*)
      read(ir,*) nnode,nelem

      call edaloc

      if(nnode.gt.maxnd) goto 9000
      if(nelem.gt.maxel) goto 9000

      read(ir,*)
      musnd=1
      idof=1

!     print *,'read node'
!     print *,nnode,nelem
      do i=1,nnode
         read(ir,*,err=9100) nn,(xnode(j,i),j=1,ndim)
         indn(i)=nn
         indnrv(nn)=i
         musnd=max0(musnd,nn)
      end do

!     print *,'read element'
      read(ir,*)
      musel=1
      do i=1,nelem
         read(ir,*) ne,(ndel(j),j=1,nnde)
         inde(i)=ne
         inderv(ne)=i
         if(ne.gt.maxel) goto 9150
         musel=max0(musel,ne)
         do j=1,nnde
            nn=indnrv(ndel(j))
            if(nn.eq.0) goto 9150
            idelm(j,i)=nn
         end do
      end do

!     print *,'read material'
      read(ir,*)
      select case(mtype)
      case(4,5)
         read(ir,*) yom,por,thckns
      case default
         read(ir,*) yom,por
      end select

!     print *,'read bc'
      read(ir,*)
      idnd=-1
      do 
         read(ir,'(a)') lline
         if( (lline(1:4).eq.'body' ).or. &
        &    (lline(1:5).eq.'press').or. &
        &    (lline(1:5).eq.'tract').or. &
        &    (lline(1:5).eq.'point').or. &
        &    (lline(1:3).eq.'end')    ) exit
         read(lline,*) nn,lbc,ful
         i=indnrv(nn)
         do j=1,nddof
            if(lbc(j).eq.1) then
               idnd(j,i)=0
               unode(j,i)=ful(j)
            end if
         end do
      end do

      idof=1
      do i=1,nnode
         do k=1,nddof
            if(idnd(k,i).ne.0) then
               idnd(k,i)=idof
               idof=idof+1
            end if
         end do
      end do
      ndof=idof-1

      if(lline(1:4).eq.'body') then
         read(ir,*) bdyf
         read(ir,'(a)') lline
      end if

      if(lline(1:5).eq.'press') then
         read(ir,*) prssr
         read(ir,'(a)') lline
      end if

      if(lline(1:5).eq.'tract') then
         read(ir,*) ntrs
!        print *,'read tractio'
         call initrv
         do i=1,ntrs
            read(ir,*) (ndel(j),j=1,nsrf),(trvse(j,i),j=1,ndim)
            do j=1,nsrf
               idtrn(j,i)=indnrv(ndel(j))
            end do
         end do
!        print *,'end traction'
         read(ir,'(a)') lline
      end if

      fnode=zero
      if(lline(1:5).eq.'point') then
         do 
            read(ir,'(a)') lline
            if(lline(1:3).eq.'end') exit
            read(lline,*) nn,ful
            i=indnrv(nn)
            do j=1,nddof
               fnode(j,i)=ful(j)
            end do
         end do
      end if

!     print *,'end rddat'
      return

 9000 write(*,*) 'error in rddat0'
      stop

 9100 write(*,*) 'error in rddat for node at count ',i
      write(*,*) nn,(xnode(j,i),j=1,ndim)
      stop
 9150 write(*,*) 'error in rddat for element'
      write(*,*) ne,(ndel(j),j=1,nnde)
      stop

end subroutine rddat

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
