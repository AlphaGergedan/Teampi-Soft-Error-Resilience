/**
 * teaMPI.cpp
 *
 * Created on 19 Aug 2019
 * Author: Philipp Samfass
 */

#include "teaMPI.h"

#include "Rank.h"

#include <mpi.h>
#include <functional>

int TMPI_GetInterTeamCommSize() {
  return getNumberOfTeams();
}

int TMPI_GetWorldRank() {
  return getWorldRank();
}

int TMPI_GetTeamNumber() {
  return getTeam();  
}

int TMPI_IsLeadingRank() {
  return getTeam()==0; 
}

MPI_Comm TMPI_GetInterTeamComm() {
  return getTeamInterComm();
}

void TMPI_SetLoadCheckpointCallback(std::function<void(void)> *function){
  setLoadCheckpointCallback(function);
}

void TMPI_SetCreateCheckpointCallback(std::function<void(void)> *function){
  setCreateCheckpointCallback(function);
}