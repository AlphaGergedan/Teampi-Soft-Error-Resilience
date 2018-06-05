/*
 * Latency.cpp
 *
 *  Created on: 5 Jun 2018
 *      Author: Ben Hazelwood
 */

#include <mpi.h>
#include <iostream>

const int NUM_TRIALS = 1e4;
const int NUM_PONGS  = 1e4;


int main(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  MPI_Barrier(MPI_COMM_WORLD);

  int rank;
  int size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  const int source = 0;
  const int dest   = size - 1;

  double m = 1.0/3.0;

  double t1 = 0.0;
  double t2 = 0.0;

  double time[NUM_TRIALS];

  MPI_Status status;

  for (int t = 0; t < NUM_TRIALS; t++) {
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == source) {
      MPI_Recv(&m, 1, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD, &status);
      t1 = MPI_Wtime();

      for (int p = 0; p < NUM_PONGS; p++) {
        MPI_Send(&m, 1, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);
        MPI_Recv(&m, 1, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD,  &status);
      }

      t2 = MPI_Wtime();
      time[t] = t2 - t1;
    }

    if (rank == dest) {
      MPI_Send(&m, 1, MPI_DOUBLE, source, 0, MPI_COMM_WORLD);
      for (int p = 0; p < NUM_PONGS; p++) {
        MPI_Recv(&m, 1, MPI_DOUBLE, source, 0, MPI_COMM_WORLD,  &status);
        MPI_Send(&m, 1, MPI_DOUBLE, source, 0, MPI_COMM_WORLD);
      }
    }
  }

  if (rank == source) {
    for (int t=0; t < NUM_TRIALS; t++) {
      std::cout << "T(" << t << ")=" << time[t] << "\n";
    }
  }


  MPI_Finalize();

return 0;
}


