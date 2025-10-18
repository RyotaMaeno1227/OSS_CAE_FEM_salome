! This file is a component of the software based on the book
! "High Performance Finite Element Method" written by
! Takahiro Yamada.
! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.

module nedata

!  Data for nodes and elements

   use kind_parameters
   use mddim

   implicit none

   integer                         :: nnode ! Number of Nodes
   integer                         :: nelem ! Number of Elements

   real(kind=DP_REAL_KIND),allocatable,dimension(:,:)   :: xnode
!                                                 Nodal Coordinates
   real(kind=DP_REAL_KIND),allocatable,dimension(:,:)   :: unode
!                                                 Nodal Displacement
   real(kind=DP_REAL_KIND),allocatable,dimension(:,:)   :: fnode
!                                                 Nodal External Force
   real(kind=DP_REAL_KIND),allocatable,dimension(:,:)   :: fint
!                                                 Nodal Internal Force

   integer,allocatable,dimension(:,:)          :: idelm
!                                                 Index of Nodal conectivity
   integer,allocatable,dimension(:,:)          :: idnd
!                                                 Index between Nodes and DOF

   integer,parameter         :: maxnd=1000000 ! Maximum number of nodes
   integer,parameter         :: maxel=1000000 ! Maximum number of elements
   integer          indn(maxnd),indnrv(maxnd)
   integer          musnd
   integer          inde(maxel),inderv(maxel)
   integer          musel

contains
   subroutine edaloc
      allocate(xnode (ndim  ,nnode )              )
      allocate(unode (nddof ,nnode )       )
      allocate(fnode (nddof ,nnode )       )
      allocate(fint  (nddof ,nnode )       )

      allocate(idelm (nnde  ,nelem )              )
      allocate(idnd  (nddof ,nnode )              )

   end subroutine edaloc
end module nedata

module ionumb

!  Unit numbers for input and output
!  and assign files

   integer,parameter   :: ir =15  ! Read unit number
   integer,parameter   :: iw =16  ! Write unit number
   integer,parameter   :: iwa=19  ! Write unit number for AVS data

   character*80 title  ! Title

contains
   subroutine openio

      open(ir ,file='fort.15',form='formatted',status='old')
      open(iwa,file='fort.16',form='formatted')
      open(iw ,file='fort.19',form='formatted')

   end subroutine openio
end module ionumb

! Copyright (c) 2006 Takahiro Yamada, All Right Reserved.
