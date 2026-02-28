program hiperfem

!  Finite Element Evaluation Plathome
!  based on the book
!  "High Performance Finite Element Method"
!  written by Takahiro Yamada.

!  Ver. 1.0 : 2006.12.14 (Public Release)

!  Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

!  FE Analysis Program main routine

   use nedata
   use cfmc
   use matmod
   use solmod
   use ionumb

   implicit none

   real(kind=DP_REAL_KIND) :: stfe (nndof,nndof)
   real(kind=DP_REAL_KIND) :: err

   integer                 :: jelm
   integer                 :: iter
   integer                 :: ierr

   call rddat

   call mkmi
   call initcf
   call caldm

   do jelm=1,nelem

      call calstf(stfe  ,jelm )
      call aseqcf(stfe  ,jelm  )

   end do

   call calfv

   call stfsol(iter  ,err   ,ierr  )

   write(*,*) 'iter,err=',iter,err

!     update velocity vector

   call udvec

   call calfi

   call etirw

   stop

end program hiperfem

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
