/**
 * teaMPI.h
 *
 * Created on 19 Aug 2019
 * Author: Philipp Samfass
 */
#ifndef TEAMPI_H_
#define TEAMPI_H_

#include <mpi.h>

MPI_Comm TMPI_GetInterTeamComm();
int TMPI_GetTeamNumber();
int TMPI_GetWorldRank();
int TMPI_GetInterTeamCommSize();
int TMPI_IsLeadingRank();

#endif