#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <set>

#include "tmpi.h"

const int MASTER = 0;

int R_FACTOR;

CommunicationModes CommMode;

ReplicationModes RepMode;

int world_rank;
int world_size;
int team_rank;
int team_size;

std::set<MPI_Request*> lut;

void read_config() {
  int temp_rank;
  int temp_size;

  std::string filename("tmpi.cfg");
  std::ifstream f(filename.c_str());
  if (!f) {
    // Config file does not exist: leave as defaults
    R_FACTOR = 2;
    CommMode = CommunicationModes::Parallel;
    RepMode = ReplicationModes::Cyclic;
    return;
  }

  std::string line;

  while(std::getline(f, line)) {
    line.erase(std::remove_if(line.begin(), line.end(), std::iswspace), line.end());
    std::transform(line.begin(), line.end(),line.begin(), ::toupper);
    std::istringstream iss(line);

    int pos = line.find_first_of('=');
    std::string key = line.substr(0,pos);
    std::string val = line.substr(pos+1);

    if (key.compare("R_FACTOR") == 0) {
      R_FACTOR = std::stoi(val);
    } else if (key.compare("R_MODE") == 0) {
      if (val.compare("CYCLIC") == 0) {
        RepMode = ReplicationModes::Cyclic;
      } else if (val.compare("ADJACENT") == 0) {
        RepMode = ReplicationModes::Adjacent;
      } else {
        std::cout << "Value (" << val << ") for " << key << " not valid\n";
      }
    } else if (key.compare("COMM_MODE") == 0) {
        if (val.compare("PARALLEL") == 0) {
          CommMode = CommunicationModes::Parallel;
        } else if (val.compare("MIRROR") == 0) {
          CommMode = CommunicationModes::Mirror;
        } else {
          std::cout << "Value (" << val << ") for " << key << " not valid\n";
        }
    } else {
      std::cout << "Key (" << key << ") not recognized\n";
      std::terminate();
    }
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

  if (dest == MASTER) {
    if (get_R_number(world_rank) == 0) {
      PMPI_Send(buf, count, datatype, MASTER, tag, comm);
    }
  } else if ((CommMode == CommunicationModes::Mirror) || (world_rank == MASTER)) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    PMPI_Recv(buf, count, datatype, source, tag, comm, status);
    remap_status(status);
  } else if (CommMode == CommunicationModes::Mirror) {
    // Not very smart right now
    // TODO array of MPI_Status
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag,
               comm, status);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
             status);
    remap_status(status);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  if (dest == MASTER) {
    if (get_R_number(world_rank) == 0) {
      PMPI_Isend(buf, count, datatype, MASTER, tag, comm, request);
    } else {
      lut.insert(request);
    }
  } else if ((CommMode == CommunicationModes::Mirror) || (world_rank == MASTER)) {
    request = (MPI_Request*)realloc(request, R_FACTOR * sizeof(MPI_Request));
    if (world_rank == MASTER) {
      lut.insert(request);
    }
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm, &request[r_num]);
    }
//    std::cout << "Request @ send: " << request << "\n";
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm,
              request);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
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
    PMPI_Irecv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
              request);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  if (CommMode == CommunicationModes::Mirror) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Wait(&request[r_num], &status[r_num]);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
        if ((lut.find(request) != lut.end()) && (world_rank == MASTER)) {;
          PMPI_Wait(request, status);
          remap_status(status);
        } else if (lut.find(request) != lut.end()) {
          if (status != MPI_STATUS_IGNORE) {
            status->MPI_SOURCE = MASTER;
          }
        } else {
          PMPI_Wait(request, status);
        }
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    PMPI_Probe(source, tag, comm, status);
    remap_status(status);
  } else if (CommMode == CommunicationModes::Mirror) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    remap_status(status);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;

}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if ((source == MASTER) || (world_rank == MASTER) || (source == MPI_ANY_SOURCE)) {
    PMPI_Iprobe(source, tag, comm, flag, status);
    remap_status(status);
  } else if (CommMode == CommunicationModes::Mirror) {
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, &r_flag, status);
      *flag &= r_flag;
    }
  } else if (CommMode == CommunicationModes::Parallel) {
    int r_num = get_R_number(world_rank);
    PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, flag, status);
    remap_status(status);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  if (CommMode == CommunicationModes::Mirror) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Test(&request[r_num], &r_flag, &status[r_num]);
    }
    *flag &= r_flag;
  } else if (CommMode == CommunicationModes::Parallel) {
    if ((lut.find(request) != lut.end()) && (world_rank == MASTER)) {
      PMPI_Test(request, flag, status);
      remap_status(status);
    } else if (lut.find(request) != lut.end()) {
      *flag = 1;
      status->MPI_SOURCE = MASTER;
    } else {
      PMPI_Test(request, flag, status);

    }

//    }
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_Finalize() {
  return PMPI_Finalize();
}
