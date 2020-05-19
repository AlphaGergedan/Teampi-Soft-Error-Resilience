#ifndef ERRORHANLDING_H_
#define ERRORHANDLING_H_

#include <mpi.h>
#include <mpi-ext.h>



void verbose_errh(MPI_Comm* pcomm, int* perr, ...);
void kill_team_errh_comm_world(MPI_Comm* pcomm, int* perr, ...);
void kill_team_errh_comm_team(MPI_Comm* pcomm, int* perr, ...);
void respawn_proc_errh_comm_world(MPI_Comm* pcomm, int* perr, ...);
void respawn_proc_errh_comm_team(MPI_Comm* pcomm, int* perr, ...);
void respawn_proc_recreate_comm_world(MPI_Comm comm);
#endif