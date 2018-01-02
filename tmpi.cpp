#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "tmpi.h"

const int MASTER = 0;

// TODO config file for options below

// Set the replication factor
const int R_FACTOR = 2;

// Communication strategies
enum COMM_MODES { PARALLEL, MIRROR};
COMM_MODES COMM_MODE = PARALLEL;

// Replication Strategies
enum REP_MODES { CYCLIC, ADJACENT};
REP_MODES REP_MODE = CYCLIC;

int world_rank;
int world_size;
int team_rank;
int team_size;



int init_rank() {
  PMPI_Comm_size(MPI_COMM_WORLD, &world_size);
  team_size = (world_size - 1) / R_FACTOR;

  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_rank = map_world_to_team(world_rank);

  return MPI_SUCCESS;
}

int get_R_number(int rank) {
  if (rank == MASTER) {
    return 0;
  }

  if (REP_MODE == CYCLIC) {
    return (rank - 1) / team_size;
  } else if (REP_MODE == ADJACENT) {
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

  if (REP_MODE == CYCLIC) {
    return ((rank - 1) % team_size) + 1;
  } else if (REP_MODE == ADJACENT) {
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

  if (REP_MODE == CYCLIC) {
    return rank + (r_num * team_size);
  } else if (REP_MODE == ADJACENT) {
    return rank + r_num;
  } else {
    assert(false);
    return -1;
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
  } else if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
    }
  } else if (COMM_MODE == PARALLEL) {
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

  if ((source == MASTER) || (world_rank == MASTER)) {
    PMPI_Recv(buf, count, datatype, source, tag, comm, status);
  } else if (COMM_MODE == MIRROR) {
    // Not very smart right now
    // TODO array of MPI_Status
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag,
               comm, status);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
             status);
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
    }
  } else if (COMM_MODE == MIRROR) {
    request = (MPI_Request *) realloc(request, R_FACTOR * sizeof(MPI_Request));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm,
                &request[r_num]);
    }
  } else if (COMM_MODE == PARALLEL) {
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

  if ((source == MASTER) || (world_rank == MASTER)) {
    PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  } else if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    request = (MPI_Request *) realloc(request, R_FACTOR * sizeof(MPI_Request));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Irecv(buf, count, datatype, map_team_to_world(source, r_num), tag,
                comm, &request[r_num]);
    }
  } else if (COMM_MODE == PARALLEL) {
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
  if (COMM_MODE == MIRROR) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Wait(&request[r_num], &status[r_num]);
    }
  } else if (COMM_MODE == PARALLEL) {
    PMPI_Wait(request, status);
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if (COMM_MODE == MIRROR) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;

}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if (COMM_MODE == MIRROR) {
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, &r_flag, status);
      *flag &= r_flag;
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, flag, status);
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  if (COMM_MODE == MIRROR) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      PMPI_Test(&request[r_num], &r_flag, &status[r_num]);
    }
    *flag &= r_flag;
  } else if (COMM_MODE == PARALLEL) {
    PMPI_Test(request, flag, status);
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_Finalize() {
  return PMPI_Finalize();
}
