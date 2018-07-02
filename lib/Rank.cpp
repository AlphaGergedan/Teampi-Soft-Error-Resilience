/*
 * RankOperations.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#include "Rank.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

#include "RankControl.h"
#include "Logging.h"
#include "Timing.h"



static int worldRank;
static int worldSize;
static int teamRank;
static int teamSize;
static int numTeams;

static MPI_Comm TMPI_COMM_TEAM;
static MPI_Comm TMPI_COMM_DUP;

int initialiseTMPI() {
  /**
   * The application should have no knowledge of the world_size or world_rank
   */
  registerSignalHandler();
  setEnvironment();

  PMPI_Comm_size(MPI_COMM_WORLD, &worldSize);
  PMPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
  teamSize = worldSize / numTeams;

  int color = worldRank / teamSize;

  PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_DUP);

  PMPI_Comm_split(MPI_COMM_WORLD, color, worldRank, &TMPI_COMM_TEAM);

  PMPI_Comm_rank(TMPI_COMM_TEAM, &teamRank);

  PMPI_Comm_size(TMPI_COMM_TEAM, &teamSize);

  assert(teamSize == (worldSize / numTeams));

  outputEnvironment();

#ifndef REPLICAS_OUTPUT
  // Disable output for all but master replica (0)
  if (getTeam() > 0) {
//    disableLogging();
  }
#endif

  Timing::initialiseTiming();

  synchroniseRanksGlobally();

  return MPI_SUCCESS;
}


int getWorldRank() {
  return worldRank;
}

int getWorldSize() {
  return worldSize;
}

int getTeamRank() {
  return teamRank;
}

int getTeamSize() {
  return teamSize;
}

int getTeam() {
  return getWorldRank() / getTeamSize();
}

int getNumberOfTeams() {
  return getWorldSize() / getTeamSize();
}

MPI_Comm getTeamComm() {
  return TMPI_COMM_TEAM;
}

int freeTeamComm() {
  return MPI_Comm_free(&TMPI_COMM_TEAM);
}

MPI_Comm getLibComm() {
  return TMPI_COMM_DUP;
}

int freeLibComm() {
  return MPI_Comm_free(&TMPI_COMM_DUP);
}

std::string getEnvString(std::string const& key)
{
    char const* val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

void outputEnvironment(){
  assert(worldSize % numTeams == 0);

  PMPI_Barrier(MPI_COMM_WORLD);
  double my_time = MPI_Wtime();
  PMPI_Barrier(MPI_COMM_WORLD);
  double times[worldSize];

  PMPI_Gather(&my_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

  PMPI_Barrier(MPI_COMM_WORLD);

  if (worldRank == MASTER) {
    std::cout << "------------TMPI SETTINGS------------\n";
    std::cout << "R_FACTOR = " << numTeams << "\n";

    std::cout << "Team size: " << teamSize << "\n";
    std::cout << "Total ranks: " << worldSize << "\n\n";


    if (worldRank == MASTER) {
      for (int i=0; i < worldSize; i++) {
        std::cout << "Tshift(" << i << "->" << mapWorldToTeamRank(i) << "->" << mapTeamToWorldRank(mapWorldToTeamRank(i),mapRankToTeamNumber(i)) << ") = " << times[i] - times[0] << "\n";
      }
    }
    std::cout << "---------------------------------------\n\n";
  }

  PMPI_Barrier(MPI_COMM_WORLD);
}

void setEnvironment() {
  std::string env(getEnvString("TEAMS"));
  numTeams = env.empty() ? 2 : std::stoi(env);
}

int mapRankToTeamNumber(int rank) {
  return rank / getTeamSize();
}

int mapWorldToTeamRank(int rank) {
  if (rank == MPI_ANY_SOURCE) {
    return MPI_ANY_SOURCE;
  } else {
    return rank % getTeamSize();
  }
}

int mapTeamToWorldRank(int rank, int r) {
  if (rank == MPI_ANY_SOURCE) {
    return MPI_ANY_SOURCE;
  }

  return rank + r * getTeamSize();
}

void remapStatus(MPI_Status *status) {
  if ((status != MPI_STATUS_IGNORE ) && (status != MPI_STATUSES_IGNORE )) {
    status->MPI_SOURCE = mapWorldToTeamRank(status->MPI_SOURCE);
    logInfo(
        "remap status source " << status->MPI_SOURCE << " to " << mapWorldToTeamRank(status->MPI_SOURCE));
  }
}

int synchroniseRanksInTeam() {
  return PMPI_Barrier(getTeamComm());
}

int synchroniseRanksGlobally() {
  return PMPI_Barrier(getLibComm());
}


