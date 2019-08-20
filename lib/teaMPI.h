/**
 * teaMPI.h
 *
 * Created on 19 Aug 2019
 * Author: Philipp Samfass
 */

#include <mpi.h>

MPI_Comm TMPI_GetInterTeamComm();
int TMPI_GetTeamNumber();
int TMPI_GetInterTeamCommSize();
