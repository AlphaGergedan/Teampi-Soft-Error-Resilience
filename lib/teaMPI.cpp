/**
 * teaMPI.cpp
 *
 * Created on 19 Aug 2019
 * Author: Philipp Samfass
 */

#include "teaMPI.h"

#include "Rank.h"

#include <mpi.h>

int TMPI_GetInterTeamCommSize() {
  return getNumberOfTeams();
}

int TMPI_GetTeamNumber() {
  return getTeam();  
}

MPI_Comm TMPI_GetInterTeamComm() {
  return getTeamInterComm();
}
