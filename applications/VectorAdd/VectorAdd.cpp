/*
 * VectorAdd.cpp
 *
 *  Created on: 10 Apr 2018
 *      Author: Ben Hazelwood Hazelwood
 *
 *  Create array on master
 *  Send to workers
 *  They add up their two arrays
 *  send back result array to master
 *  finalize
 */
#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include "../../lib/teaMPI.h"

#ifdef TMPI
#include "../../lib/Timing.h"

#endif

const int MASTER = 0;
const int DATA_LENGTH = 3e7;

#define TMPI

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);

  int rank, size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  std::cout << TMPI_GetTeamNumber() << std::endl;
  const int NUM_WORKERS = (size - 1);

  assert((DATA_LENGTH % (size - 1)) == 0);

  const int WORKER_LENGTH = DATA_LENGTH / NUM_WORKERS;

  // Initialise arrays
  double *a,*b,*c;
  if (rank == MASTER) {
    a = (double*)malloc(DATA_LENGTH * sizeof(double));
    b = (double*)malloc(DATA_LENGTH * sizeof(double));
    c = (double*)malloc(DATA_LENGTH * sizeof(double));

    for (int i = 0; i < DATA_LENGTH; i++) {
      a[i] = (double)rand() / RAND_MAX;
      b[i] = (double)rand() / RAND_MAX;
      c[i] = 0.0;
    }

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Master initialised arrays\n";

    // Send data to workers
    MPI_Request send_reqs[NUM_WORKERS*2];
    for (int i = 1; i < size; i++) {
      int offset = (i-1) * WORKER_LENGTH;
      MPI_Isend(a+offset, WORKER_LENGTH, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &send_reqs[(i-1)*2]);
      MPI_Isend(b+offset, WORKER_LENGTH, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &send_reqs[(i-1)*2+1]);
      
    }

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Master initialised Isends\n";
    MPI_Waitall(NUM_WORKERS*2, send_reqs, MPI_STATUSES_IGNORE);
    std::cout << "Master finished sending\n";

#ifdef TMPI
    MPI_Wtime();
#endif

    // Receive results from workers
    MPI_Request recv_reqs[NUM_WORKERS];
    for (int i = 1; i < size; i++) {
      int offset = (i-1) * WORKER_LENGTH;
      MPI_Irecv(c+offset, WORKER_LENGTH, MPI_DOUBLE, i, 2, MPI_COMM_WORLD, &recv_reqs[(i-1)]);
    }

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Master initialised Irecv\n";
    MPI_Waitall(NUM_WORKERS, recv_reqs, MPI_STATUSES_IGNORE);

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Master finished receiving\n";

  } else {
    a = (double*)malloc(WORKER_LENGTH * sizeof(double));
    b = (double*)malloc(WORKER_LENGTH * sizeof(double));
    c = (double*)malloc(WORKER_LENGTH * sizeof(double));

    for (int i = 0; i < WORKER_LENGTH; i++) {
      a[i] = 0.0;
      b[i] = 0.0;
      c[i] = 0.0;
    }

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Worker initialised arrays\n";

    MPI_Request recv_reqs[2];
    MPI_Irecv(a, WORKER_LENGTH, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD, &recv_reqs[0]);
    MPI_Irecv(b, WORKER_LENGTH, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD, &recv_reqs[1]);

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Worker initialised receive\n";
    MPI_Waitall(2, recv_reqs, MPI_STATUSES_IGNORE);

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Worker received arrays\n";

    for (int i = 0; i < WORKER_LENGTH; i++) {
      c[i] = a[i] + b[i];
      if(rank == 1 && TMPI_GetTeamNumber() == 1){
       
          std::cout << "OH no failure" << std::endl;
          raise(SIGKILL);
      }
    }

#ifdef TMPI
    MPI_Wtime();
#endif

    MPI_Request send_req;
    MPI_Isend(c, WORKER_LENGTH, MPI_DOUBLE, MASTER, 2, MPI_COMM_WORLD, &send_req);

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Worker initialised send\n";
    MPI_Wait(&send_req, MPI_STATUS_IGNORE);

#ifdef TMPI
    MPI_Wtime();
#endif

    std::cout << "Worker completed send\n";
  }

  std::cout << "Master at finalise\n";
  MPI_Finalize();
}







