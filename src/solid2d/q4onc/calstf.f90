! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

subroutine calstf(stfe  ,jelm  )

!  Generate and solve matrix equation

   use nedata
   use upelm
   use cfmc
   use matmod
   use dconst

   implicit none

   real(kind=DP_REAL_KIND),intent(out) :: stfe (nndof,nndof)

   integer                ,intent(in)  :: jelm

   integer                 :: i,j

   do i=1,nnde
      j=idelm(i,jelm)
      xel(:,i)=xnode(:,j)
   end do

   stfet=zero

   do intg=1,nint

      call shcoef(jelm)

      call cestf

   end do

   call elmncm(stfe)

end subroutine calstf


! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
