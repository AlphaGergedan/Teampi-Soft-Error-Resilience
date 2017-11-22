#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "Wrapper.h"

#define MASTER 0

// TODO config file for options below

// Set the replication factor
#define R_FACTOR 2

// Communication strategies
#define PARALLEL 0
#define MIRROR 1
#define COMM_MODE PARALLEL

// Replication Strategies {CYCLIC, ADJACENT}
#define CYCLIC 0
#define ADJACENT 1
#define REP_MODE CYCLIC

int world_rank;
int world_size;
int team_rank;
int team_size;

int init_rank() {
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  team_size = (world_size - 1) / R_FACTOR;

  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
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

int TMPI_Init(int *argc, char*** argv) {
  MPI_Init(argc, argv);

  init_rank();

  return MPI_SUCCESS;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ) {
  MPI_Init_thread(argc, argv, required, provided);

  init_rank();

  return MPI_SUCCESS;
}

int TMPI_Comm_rank(MPI_Comm comm, int *rank) {
  assert(comm == MPI_COMM_WORLD);

  *rank = team_rank;

  return MPI_SUCCESS;
}

int TMPI_Comm_size(MPI_Comm comm, int *size) {
  assert(comm == MPI_COMM_WORLD);

  *size = team_size + 1;

  return MPI_SUCCESS;
}

int TMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm) {
  assert(comm == MPI_COMM_WORLD);

  if (dest == MASTER) {
    MPI_Send(buf, count, datatype, MASTER, tag, comm);
  } else if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    MPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int TMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if (source == MASTER) {
    MPI_Recv(buf, count, datatype, MASTER, tag, comm, status);
  } else if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    // Not very smart right now
    // TODO array of MPI_Status
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag,
               comm, status);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    MPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
             status);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int TMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  if (dest == MASTER) {
    MPI_Isend(buf, count, datatype, MASTER, tag, comm, request);
  } else if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    request = (MPI_Request *) realloc(request, R_FACTOR * sizeof(MPI_Request));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm,
                &request[r_num]);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    MPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm,
              request);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int TMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  if (source == MASTER) {
    MPI_Irecv(buf, count, datatype, MASTER, tag, comm, request);
  } else if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    request = (MPI_Request *) realloc(request, R_FACTOR * sizeof(MPI_Request));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Irecv(buf, count, datatype, map_team_to_world(source, r_num), tag,
                comm, &request[r_num]);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    MPI_Irecv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm,
              request);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;
}

int TMPI_Wait(MPI_Request *request, MPI_Status *status) {
  if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Wait(&request[r_num], &status[r_num]);
    }
  } else if (COMM_MODE == PARALLEL) {
    MPI_Wait(request, status);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int TMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    MPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
  } else {
    assert(false);
    return -1;
  }

  return MPI_SUCCESS;

}

int TMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Iprobe(map_team_to_world(source, r_num), tag, comm, &r_flag, status);
      *flag &= r_flag;
    }
  } else if (COMM_MODE == PARALLEL) {
    int r_num = get_R_number(world_rank);
    MPI_Iprobe(map_team_to_world(source, r_num), tag, comm, flag, status);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int TMPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  if ((COMM_MODE == MIRROR) || (world_rank == MASTER)) {
    status = (MPI_Status *) realloc(status, R_FACTOR * sizeof(MPI_Status));
    int r_flag = 0;
    for (int r_num = 0; r_num < R_FACTOR; r_num++) {
      MPI_Test(&request[r_num], &r_flag, &status[r_num]);
    }
    *flag &= r_flag;
  } else if (COMM_MODE == PARALLEL) {
    MPI_Test(request, flag, status);
  } else {
    assert(false);
    return -1;
  }
  return MPI_SUCCESS;
}

int TMPI_Finalize() {
  return MPI_Finalize();
}
