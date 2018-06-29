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
#include <csignal>
#include <unistd.h>

#include "Logging.h"
#include "Timing.h"


static int R_FACTOR;
static MPI_Comm TMPI_COMM_REP;
static MPI_Comm TMPI_COMM_WORLD;
static int world_rank;
static int world_size;
static int team_rank;
static int team_size;


int initaliseTMPI() {
  /**
   * The application should have no knowledge of the world_size or world_rank
   */

  signal(SIGUSR1, pauseThisRankSignalHandler);
  setEnvironment();

  PMPI_Comm_size(MPI_COMM_WORLD, &world_size);
  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_size = world_size / R_FACTOR;

  int color = world_rank / team_size;

  PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_WORLD);

  PMPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &TMPI_COMM_REP);

  PMPI_Comm_rank(TMPI_COMM_REP, &team_rank);

  PMPI_Comm_size(TMPI_COMM_REP, &team_size);

  assert(team_size == (world_size / R_FACTOR));

  outputEnvironment();

#ifndef REPLICAS_OUTPUT
  // Disable output for all but master replica (0)
  if (getTeam() > 0) {
    Logging::disableLogging();
  }
#endif

  Timing::initialiseTiming();

  PMPI_Barrier(getLibComm());

  return MPI_SUCCESS;
}


int getWorldRank() {
  return world_rank;
}

int getWorldSize() {
  return world_size;
}

int getTeamRank() {
  return team_rank;
}

int getTeamSize() {
  return team_size;
}

int getTeam() {
  return getWorldRank() / getTeamSize();
}

int getNumberOfTeams() {
  return getWorldSize() / getTeamSize();
}

MPI_Comm getTeamComm() {
  return TMPI_COMM_REP;
}

int freeTeamComm() {
  return MPI_Comm_free(&TMPI_COMM_REP);
}

MPI_Comm getLibComm() {
  return TMPI_COMM_WORLD;
}

int freeLibComm() {
  return MPI_Comm_free(&TMPI_COMM_WORLD);
}




std::string getEnvString(std::string const& key)
{
    char const* val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

void outputEnvironment(){
  assert(world_size % R_FACTOR == 0);

  PMPI_Barrier(MPI_COMM_WORLD);
  double my_time = MPI_Wtime();
  PMPI_Barrier(MPI_COMM_WORLD);
  double times[world_size];

  PMPI_Gather(&my_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

  PMPI_Barrier(MPI_COMM_WORLD);

  if (world_rank == MASTER) {
    std::cout << "------------TMPI SETTINGS------------\n";
    std::cout << "R_FACTOR = " << R_FACTOR << "\n";

    std::cout << "Team size: " << team_size << "\n";
    std::cout << "Total ranks: " << world_size << "\n\n";


    if (world_rank == MASTER) {
      for (int i=0; i < world_size; i++) {
        std::cout << "Tshift(" << i << "->" << mapWorldToTeamRank(i) << "->" << mapTeamToWorldRank(mapWorldToTeamRank(i),mapRankToTeamNumber(i)) << ") = " << times[i] - times[0] << "\n";
      }
    }
    std::cout << "---------------------------------------\n\n";
  }

  PMPI_Barrier(MPI_COMM_WORLD);
}

void setEnvironment() {
  std::string env;

  env = getEnvString("R_FACTOR");
  R_FACTOR = env.empty() ? 2 : std::stoi(env);

}

void pauseThisRankSignalHandler( int signum ) {
  const int startValue = 1e4;
  const int multiplier = 2;
  static int sleepLength = startValue;
  logDebug( "Signal received: sleep for " << (double)sleepLength / 1e6 << "s");
  usleep(sleepLength);
  sleepLength *= multiplier;
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

