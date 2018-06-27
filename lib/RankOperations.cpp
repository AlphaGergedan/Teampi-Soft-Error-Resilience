/*
 * RankOperations.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#include "RankOperations.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <csignal>
#include <unistd.h>

#include "Logging.h"
#include "Timing.h"
#include "TMPIConstants.h"


static int R_FACTOR;
static MPI_Comm TMPI_COMM_REP;
static MPI_Comm TMPI_COMM_WORLD;
static int world_rank;
static int world_size;
static int team_rank;
static int team_size;

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

int getNumberOfReplicas() {
  return R_FACTOR;
}

MPI_Comm getReplicaCommunicator() {
  return TMPI_COMM_REP;
}

int freeReplicaCommunicator() {
  return MPI_Comm_free(&TMPI_COMM_REP);
}

MPI_Comm getTMPICommunicator() {
  return TMPI_COMM_WORLD;
}

int freeTMPICommunicator() {
  return MPI_Comm_free(&TMPI_COMM_WORLD);
}

std::string getEnvString(std::string const& key)
{
    char const* val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

void print_config(){
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
        std::cout << "Tshift(" << i << "->" << map_world_to_team(i) << "->" << map_team_to_world(map_world_to_team(i),get_R_number(i)) << ") = " << times[i] - times[0] << "\n";
      }
    }
    std::cout << "---------------------------------------\n\n";
  }

  PMPI_Barrier(MPI_COMM_WORLD);
}

void read_config() {
  std::string env;

  env = getEnvString("R_FACTOR");
  R_FACTOR = env.empty() ? 2 : std::stoi(env);

}

void signalHandler( int signum ) {
  std::cout << "Signal" << signum << " received --> sleep for 10s\n";
  usleep(1e7);
}


int init_rank() {
  /**
   * The application should have no knowledge of the world_size or world_rank
   */

  signal(SIGUSR1, signalHandler);
  read_config();

  PMPI_Comm_size(MPI_COMM_WORLD, &world_size);
  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_size = world_size / R_FACTOR;

  int color = world_rank / team_size;

  PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_WORLD);

  PMPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &TMPI_COMM_REP);

  PMPI_Comm_rank(TMPI_COMM_REP, &team_rank);

  PMPI_Comm_size(TMPI_COMM_REP, &team_size);

  assert(team_size == (world_size / R_FACTOR));

  print_config();

  int r_num = get_R_number(world_rank);
  assert(world_rank == map_team_to_world(map_world_to_team(world_rank), r_num));

  // Usually we do not want std::cout from all replicas
#ifndef REPLICAS_OUTPUT
  if (get_R_number(world_rank) > 0) {
//    std::cout.setstate(std::ios_base::failbit);
//    std::cerr.setstate(std::ios_base::failbit);
  }
#endif

  Timing::markTimeline(Timing::markType::Initialize);

  PMPI_Barrier(getTMPICommunicator());

  return MPI_SUCCESS;
}

int get_R_number(int rank) {
  return rank / team_size;
}


int map_world_to_team(int rank) {
  if (rank == MPI_ANY_SOURCE) {
    return MPI_ANY_SOURCE;
  } else {
    return rank % team_size;
  }
}


int map_team_to_world(int rank, int r_num) {
  if (rank == MPI_ANY_SOURCE) {
    return MPI_ANY_SOURCE;
  }

  if (r_num < 0) {
    r_num = get_R_number(world_rank);
  }

  return rank + r_num * team_size;
}


void remap_status(MPI_Status *status) {
  if ((status != MPI_STATUS_IGNORE) && (status != MPI_STATUSES_IGNORE)) {
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
    logInfo("remap status source " << status->MPI_SOURCE << " to " << map_world_to_team(status->MPI_SOURCE));
  }
}


