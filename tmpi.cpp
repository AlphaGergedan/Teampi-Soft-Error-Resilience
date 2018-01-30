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
  team_size = (world_size - 1) / R_FACTOR;

  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_rank = map_world_to_team(world_rank);

  print_config();

  return MPI_SUCCESS;
}

int get_R_number(int rank) {
  if (rank == MASTER) {
    return 0; // We do not replicate the master rank
  }

  if (RepMode == ReplicationModes::Cyclic) {
    return (rank - 1) / team_size;
  } else if (RepMode == ReplicationModes::Adjacent) {
    return (rank - 1) % R_FACTOR;
  } else {
    assert(false);
    return -1;
  }
}

int map_world_to_team(int rank) {
  if (rank == MASTER) {
    return 0;
  }

  if (RepMode == ReplicationModes::Cyclic) {
    return ((rank - 1) % team_size) + 1;
  } else if (RepMode == ReplicationModes::Adjacent) {
    return ((rank - 1) / R_FACTOR) + 1;
  } else {
    assert(false);
    return -1;
  }
}

int map_team_to_world(int rank, int r_num) {
  if (rank == MASTER) {
    return 0;
  }

  if (RepMode == ReplicationModes::Cyclic) {
    return rank + (r_num * team_size);
  } else if (RepMode == ReplicationModes::Adjacent) {
    return rank + r_num;
  } else {
    assert(false);
    return -1;
  }
}

void remap_status(MPI_Status *status) {
  if (status != MPI_STATUS_IGNORE) {
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  }
}

int MPI_Init(int *argc, char*** argv) {
  PMPI_Init(argc, argv);

  init_rank();

  return MPI_SUCCESS;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ) {
  PMPI_Init_thread(argc, argv, required, provided);

  init_rank();

  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  assert(comm == MPI_COMM_WORLD);

  *rank = team_rank;

  return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  assert(comm == MPI_COMM_WORLD);

  *size = team_size + 1;

  return MPI_SUCCESS;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm) {
  assert(comm == MPI_COMM_WORLD);
  int err = 0;
  if (dest == MASTER) {
    if (get_R_number(world_rank) == 0) {
      err |= PMPI_Send(buf, count, datatype, MASTER, tag, comm);
    }
  } else if ((CommMode == CommunicationModes::Mirror) || (world_rank == MASTER)) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      err |= PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    err |= PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
  } else {
    assert(false);
    return -1;
  }

  return err;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    err |= PMPI_Recv(buf, count, datatype, source, tag, comm, status);
    remap_status(status);
  } else if (CommMode == CommunicationModes::Mirror) {
    // Not very smart right now
    // TODO array of MPI_Status
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      err |= PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag,
               comm, status);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    err |= PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
             status);
    remap_status(status);
  } else {
    assert(false);
    return -1;
  }

  return err;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  if (dest == MASTER) {
    if (get_R_number(world_rank) == 0) {
      err |= PMPI_Isend(buf, count, datatype, MASTER, tag, comm, request);
    } else {
      lut.insert(std::map<MPI_Request*, std::vector<MPI_Request*>>::value_type(request, std::vector<MPI_Request*>()));
    }
  } else if ((CommMode == CommunicationModes::Mirror) || (world_rank == MASTER)) {
    if (world_rank == MASTER) {
      lut.insert(std::map<MPI_Request*, std::vector<MPI_Request*>>::value_type(request, std::vector<MPI_Request*>(R_FACTOR-1)));
    }
    for (int r = 0; r < R_FACTOR; r++) {
      MPI_Request *req;
      if (r > 0) {
        req = new MPI_Request();
        lut[request][r-1] = req;
      } else {
        req = request;
      }
      err |= PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r), tag, comm, req);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    err |= PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm,
              request);
  } else {
    assert(false);
    return -1;
  }
  return err;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    err |= PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  } else if (CommMode == CommunicationModes::Mirror) {
////    MPI_Request* new_request = (MPI_Request *)malloc(R_FACTOR * sizeof(MPI_Request));
////    request = new_request;
//    request = (MPI_Request *) realloc(request, R_FACTOR * sizeof(MPI_Request));
//    for (int i=0; i < R_FACTOR; i++) {
//      request[i] = new MPI_Request();
//    }
//    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
//      PMPI_Irecv(buf, count, datatype, map_team_to_world(source, r_num), tag,
//                comm, &request[r_num]);
//    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    source = map_team_to_world(source, r_num);
    err |= PMPI_Irecv(buf, count, datatype, source, tag, comm,
              request);
  } else {
    assert(false);
    return -1;
  }

  return err;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  int err = 0;
  if (CommMode == CommunicationModes::Mirror) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      err |= PMPI_Wait(&request[r_num], &status[r_num]);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
        if ((lut.find(request) != lut.end()) && (world_rank == MASTER)) {
          err |= PMPI_Wait(request, status);
          remap_status(status);
          err |= PMPI_Waitall(R_FACTOR-1, lut.at(request)[0], MPI_STATUS_IGNORE);
        } else if (lut.find(request) != lut.end()) {
          if (status != MPI_STATUS_IGNORE) {
            status->MPI_SOURCE = MASTER;
          }
        } else {
          err |= PMPI_Wait(request, status);
          remap_status(status);
        }

  } else {
    assert(false);
    return -1;
  }
  return err;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);
  int err = 0;
  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    err |= PMPI_Probe(source, tag, comm, status);
    remap_status(status);
  } else if (CommMode == CommunicationModes::Mirror) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      err |= PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    err |= PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    remap_status(status);

  } else {
    assert(false);
    return -1;
  }

  return err;

}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);
  assert(!((source == MPI_ANY_SOURCE) && (tag == MPI_ANY_TAG)));

  int err = 0;

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    err |= PMPI_Iprobe(source, tag, comm, flag, status);
    remap_status(status);
  } else if (CommMode == CommunicationModes::Mirror) {
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      err |= PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, &r_flag, status);
      *flag &= r_flag;
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    err |= PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, flag, status);
    remap_status(status);

  } else {
    assert(false);
    return -1;
  }
  return err;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  int err = 0;
  if (CommMode == CommunicationModes::Mirror) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      err |= PMPI_Test(&request[r_num], &r_flag, &status[r_num]);
    }
    *flag &= r_flag;
  } else if (CommMode == CommunicationModes::Parallel) {
    if ((lut.find(request) != lut.end()) && (world_rank == MASTER)) {
      err |= PMPI_Test(request, flag, status);
      remap_status(status);
      err |= PMPI_Testall(R_FACTOR-1, lut.at(request)[0],flag, MPI_STATUS_IGNORE);
    } else if (lut.find(request) != lut.end()) {
      *flag = 1;
      status->MPI_SOURCE = MASTER;
    } else {
      err |= PMPI_Test(request, flag, status);
      remap_status(status);
    }

  } else {
    assert(false);
    return -1;
  }
  return err;
}

int MPI_Finalize() {
  return PMPI_Finalize();
}
