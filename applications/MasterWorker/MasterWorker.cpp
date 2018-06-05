/*
 * MasterWorker.cpp
 *
 *  Created on: 5 Apr 2018
 *      Author: Ben Hazelwood
 */
#include <iostream>
#include <mpi.h>
#include <math.h>

const int MASTER = 0;

const int DATA_LENGTH = 1e7;

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  double* data = (double*) malloc(DATA_LENGTH * sizeof(double));
  if (!data) {
    std::cout << "Couldn't allocate data array\n";
    std::terminate();
  }

  if (rank == MASTER) {
    /*
     * Algorithm idea
     * Master picks a (could be random but fixed for now) number indicating array size
     * Sends to worker
     * Worker computes a reduction of sines of array
     * Sends a confirmation back to master
     * Master replies with another array
     * Repeat
     */
    const int numWorkers = size - 1;

    for (int i = 0; i < DATA_LENGTH; i++) {
      data[i] = 1.0/3.0;
    }

    MPI_Request requests[numWorkers];
    MPI_Status statuses[numWorkers];

    for (int i = 1; i < size; i++) {
      MPI_Isend(data, DATA_LENGTH, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, &requests[i-1]);
    }

    MPI_Waitall(numWorkers, requests, statuses);

    double results[numWorkers];
    for (int i = 1; i < size; i++) {
      requests[i-1] = MPI_Request();
      MPI_Irecv(&results[i-1], 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &requests[i-1]);
    }
    MPI_Waitall(numWorkers, requests, statuses);

    free(data);


    std::cout << "Master finished\n";
  }
  else {
    MPI_Recv(data, DATA_LENGTH, MPI_DOUBLE, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    double result = 0.0;
    for (int i=0; i < DATA_LENGTH; i++) {
      result += sin(data[i]);
    }

    MPI_Send(&result, 1, MPI_DOUBLE, MASTER, 1, MPI_COMM_WORLD);

    free(data);

    std::cout << "Worker " << rank - 1 << " finished\n";
  }


  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();

  return 0;
}


