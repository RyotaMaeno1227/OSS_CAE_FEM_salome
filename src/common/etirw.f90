! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

subroutine etirw

!     write model data

      use ionumb
      use nedata
      use solmod
      use cfmc
      use matmod
      use dconst
      use upelm

      implicit none

      real(kind=DP_REAL_KIND),dimension(nstrss,nires) :: stress

      integer          i,j
      integer          jelm

      call setxlp

!     Displacement
      write(iw,8000) 'Displacement'
      select case(mtype)
      case(4,5)
         write(iw,8100) 'Node','w','t1','t2'
      case(3)
         write(iw,8100) 'Node','u1','u2','u3'
      case default
         write(iw,8100) 'Node','u1','u2'
      end select
      do i=1,nnode
         write(iw,8200) indn(i),(unode(j,i),j=1,nddof)
      end do

      write(iw,*)
       
!     Stress

      write(iw,8000) 'Stress'

      select case(mtype)
      case(4)
         write(iw,8100) 'Element',&
        &               'M11','M22','M12','G13','G23'
      case(5)
         write(iw,8100) 'Element',&
        &               'M11','M22','M12'
      case(3)
         write(iw,8100) 'Element',&
        &               'S11','S22','S33','S12','S23','S31'

      case default
         write(iw,8100) 'Element',&
        &               'S11','S22','S12','S33'
      end select

      do jelm=1,nelem

         call css(stress,jelm)

         select case(mtype)
         case(1)
            write(iw,8300) inde(jelm),(stress(i,1),i=1,nstrss)
            if(nires.gt.1) then
               do intg=2,nires
                  write(iw,8310) (stress(i,intg),i=1,nstrss)
               end do
            end if
         case(2)
            write(iw,8300) inde(jelm),(stress(i,1),i=1,nstr),zero
            if(nires.gt.1) then
               do intg=2,nires
                  write(iw,8310) (stress(i,intg),i=1,nstr),zero
               end do
            end if
         case default
            write(iw,8300) inde(jelm),(stress(i,1),i=1,nstr)
            if(nires.gt.1) then
               do intg=2,nires
                  write(iw,8310) (stress(i,intg),i=1,nstr)
               end do
            end if
         end select
      end do

!     Internal Force

      write(iw,8000) 'Internal Force'
  
      select case(mtype)
      case(4,5)
         write(iw,8100) 'Node','Fint_1','Mint_1','Mint_2'
      case(3)
         write(iw,8100) 'Node','Fint_1','Fint_2','Fint_3'
      case default
         write(iw,8100) 'Node','Fint_1','Fint_2' 
      end select

      do i=1,nnode
         write(iw,8200) indn(i),(fint(j,i),j=1,nddof)
      end do

 8000 format(a)
 8100 format(a8,2x,6(a10,6x))
 8200 format(i8,2x,1p,3g16.7)
 8300 format(i8,2x,1p,6g16.7)
 8310 format(10x,1p,6g16.7)

end subroutine etirw

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
