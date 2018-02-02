#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <vector>

#include "tmpi.h"

const int MASTER = 0;

int R_FACTOR;

CommunicationModes CommMode;

ReplicationModes RepMode;

int world_rank;
int world_size;
int team_rank;
int team_size;

std::map<MPI_Request*, std::vector<MPI_Request*>> lut;

std::string getEnvString(std::string const& key)
{
    char const* val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

void read_config() {
  std::string env;

  env = getEnvString("R_FACTOR");
  R_FACTOR = env.empty() ? 2 : std::stoi(env);

  env = getEnvString("R_MODE");
  if (env.compare("CYCLIC") == 0) {
    RepMode = ReplicationModes::Cyclic;
  } else if (env.compare("ADJACENT") == 0) {
    RepMode = ReplicationModes::Adjacent;
  } else if (env.empty()){
    RepMode = ReplicationModes::Cyclic;
  } else{
    std::cout << "Value (" << env << ") for " << "R_MODE" << " not valid\n";
  }

  env = getEnvString("COMM_MODE");
  if (env.compare("PARALLEL") == 0) {
    CommMode = CommunicationModes::Parallel;
  } else if (env.compare("MIRROR") == 0) {
    CommMode = CommunicationModes::Mirror;
  } else if (env.empty()){
    CommMode = CommunicationModes::Parallel;
  } else{
    std::cout << "Value (" << env << ") for " << "COMM_MODE" << " not valid\n";
  }
}

void print_config(){
  MPI_Barrier(MPI_COMM_WORLD);

  if (world_rank == MASTER) {
    std::cout << "------------TMPI SETTINGS------------\n";
    std::cout << "R_FACTOR = " << R_FACTOR << "\n";

    std::cout << "REP_MODE = ";
    if (RepMode == ReplicationModes::Cyclic) {
      std::cout << "CYCLIC\n";
    } else {
      std::cout << "ADJACENT\n";
    }

    std::cout << "COMM_MODE = ";
    if (CommMode == CommunicationModes::Mirror) {
      std::cout << "MIRROR\n";
    } else {
      std::cout << "PARALLEL\n";
    }

    std::cout << "Team size: " << team_size << "\n";
    std::cout << "Total ranks: " << world_size << "\n";
    std::cout << "--------------------------------------\n\n";
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

int init_rank() {
  /**
   * The application should have no knowledge of the world_size or world_rank
   */
  read_config();

  PMPI_Comm_size(MPI_COMM_WORLD, &world_size);
  team_size = (world_size) / R_FACTOR;

  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_rank = map_world_to_team(world_rank);

  print_config();

  return MPI_SUCCESS;
}

int get_R_number(int rank) {
  switch(RepMode) {
    case ReplicationModes::Cyclic:
      return rank / team_size;
    case ReplicationModes::Adjacent:
      return rank % R_FACTOR;
    default:
      assert(false);
      return -1;
  }
}

int map_world_to_team(int rank) {
  switch(RepMode) {
    case ReplicationModes::Cyclic:
      return rank % team_size;
    case ReplicationModes::Adjacent:
      return rank / R_FACTOR;
    default:
      assert(false);
      return -1;
  }
}

int map_team_to_world(int rank, int r_num) {
  switch(RepMode) {
    case ReplicationModes::Cyclic:
      return rank + (r_num * team_size);
    case ReplicationModes::Adjacent:
      return rank + r_num;
    default:
      assert(false);
      return -1;
  }
}

void remap_status(MPI_Status *status) {
  if (status != MPI_STATUS_IGNORE) {
    //logDebug("remap status source " << status->MPI_SOURCE << " to " << map_world_to_team(status->MPI_SOURCE));
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  }
}

int MPI_Init(int *argc, char*** argv) {
  PMPI_Init(argc, argv);

  init_rank();

  logDebug("Initialised");

  return MPI_SUCCESS;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ) {
  PMPI_Init_thread(argc, argv, required, provided);

  init_rank();
  logDebug("Initialised");

  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  assert(comm == MPI_COMM_WORLD);

  *rank = team_rank;
  logDebug("Returning rank " << *rank);

  return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  assert(comm == MPI_COMM_WORLD);

  *size = team_size;
  logDebug("Returning size " << *size);

  return MPI_SUCCESS;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm) {
  assert(comm == MPI_COMM_WORLD);
  int err = 0;
  //  if (world_rank != team_rank) {
  //    logDebug("HERE");
  //  }
  int r_num = get_R_number(world_rank);
  err |= PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
  logDebug("Send to rank " << map_team_to_world(dest, r_num) << " with tag " << tag);

  return err;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
           status);
  logDebug("Receive from rank " << map_team_to_world(source, r_num) << " with tag " << tag);
  remap_status(status);

  return err;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm,
            request);
  logDebug("Isend to rank " << map_team_to_world(dest, r_num) << " with tag " << tag);
  return err;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  source = map_team_to_world(source, r_num);
  err |= PMPI_Irecv(buf, count, datatype, source, tag, comm,
            request);
  logDebug("Ireceive from rank " << map_team_to_world(source, r_num) << " with tag " << tag);

  return err;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  int err = 0;

  logDebug("Wait initialised");
  err |= PMPI_Wait(request, status);
  remap_status(status);
  logDebug("Wait completed");

  return err;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);
  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
  remap_status(status);

  return err;

}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, flag, status);
  remap_status(status);

  return err;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  int err = 0;

  err |= PMPI_Test(request, flag, status);
  remap_status(status);

  return err;
}

int MPI_Finalize() {
  return PMPI_Finalize();
}
