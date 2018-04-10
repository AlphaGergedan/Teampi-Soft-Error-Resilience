/*
 * RankOperations.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: ben
 */

#include "RankOperations.h"
#include "TMPIConstants.h"
#include "Timing.h"
#include "Logging.h"

#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <stddef.h>


static int R_FACTOR;
static MPI_Comm MPI_COMM_REP;
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

MPI_Comm getCommunicator() {
  return MPI_COMM_REP;
}

int freeCommunicator() {
  return MPI_Comm_free(&MPI_COMM_REP);
}

std::string getEnvString(std::string const& key)
{
    char const* val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

void print_config(){
  assert(world_size % R_FACTOR == 0);

  MPI_Barrier(MPI_COMM_WORLD);
  double my_time = MPI_Wtime();
  MPI_Barrier(MPI_COMM_WORLD);
  double times[world_size];

  MPI_Gather(&my_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

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

  MPI_Barrier(MPI_COMM_WORLD);
}

void output_timing() {

  char sep = ',';
  std::ostringstream filename;
  filename << "timings-" << world_rank << "-" << team_rank << "-" << get_R_number(world_rank) << ".csv";
  std::ofstream f;
  f.open(filename.str().c_str());

  f << "iSendStart";
  for (const auto& t : Timing::getISendStartTimes()) {
    f << "," << t;
  }
  f << "\n";

  f << "iSendEnd";
  for (const auto& t : Timing::getISendEndTimes()) {
    f << "," << t;
  }
  f << "\n";

  f << "iRecvStart";
  for (const auto& t : Timing::getIRecvStartTimes()) {
    f << "," << t;
  }
  f << "\n";

  f << "iRecvEnd";
  for (const auto& t : Timing::getIRecvEndTimes()) {
    f << "," << t;
  }
  f << "\n";

  f.close();
  MPI_Barrier(MPI_COMM_WORLD);
  for (int i = 0; i < world_size; i++) {
    if (i == world_rank) {
      std::cout << "RANK " << i << "\n";
      std::cout << "iSendStartLog: " << Timing::getISendStartTimes().size() << "\n";
      std::cout << "iSendEndLog: " << Timing::getISendEndTimes().size() << "\n";
      std::cout << "iRecvStartLog: " << Timing::getIRecvStartTimes().size() << "\n";
      std::cout << "iRecvEndLog: " << Timing::getIRecvEndTimes().size() << "\n";
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }
}

void read_config() {
  std::string env;

  env = getEnvString("R_FACTOR");
  R_FACTOR = env.empty() ? 2 : std::stoi(env);

}

int init_rank() {
  /**
   * The application should have no knowledge of the world_size or world_rank
   */
  Timing::initialise();

  read_config();

  PMPI_Comm_size(MPI_COMM_WORLD, &world_size);
  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_size = world_size / R_FACTOR;

  int color = world_rank / team_size;

  PMPI_Comm_split(MPI_COMM_WORLD, color, world_rank, &MPI_COMM_REP);

  PMPI_Comm_rank(MPI_COMM_REP, &team_rank);

  PMPI_Comm_size(MPI_COMM_REP, &team_size);

  assert(team_size == (world_size / R_FACTOR));

  print_config();

  int r_num = get_R_number(world_rank);
  assert(world_rank == map_team_to_world(map_world_to_team(world_rank), r_num));

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
  } else {
    return rank + r_num * team_size;
  }
}


void remap_status(MPI_Status *status) {
  if (status != MPI_STATUS_IGNORE) {
    logInfo("remap status source " << status->MPI_SOURCE << " to " << map_world_to_team(status->MPI_SOURCE));
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  }
}


