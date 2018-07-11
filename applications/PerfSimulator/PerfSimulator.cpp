/*
 * PerfSimulator.cpp
 *
 *  Created on: 5 Jun 2018
 *      Author: Ben Hazelwood
 */

#include <mpi.h>
#include <iostream>
#include <assert.h>
#include <cstdlib>
#include <math.h>  

const int NUM_TRIALS = 100  ;
const int NUM_COMPUTATIONS = 5e7;

int main(int argc, char *argv[])
{

  MPI_Init(&argc, &argv);

  // if (argc != 3)
  // {
  //   std::cout << "Usage: mpirun Latency <NUM TRIALS> <NUM COMPUTATIONS>\n";
  //   MPI_Abort(MPI_COMM_WORLD, 1);
  // }

  int rank;
  int size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  const int source = 0;
  const int dest = size - 1;

  char m = '0';

  for (int t = 0; t < NUM_TRIALS; t++)
  {
    MPI_Barrier(MPI_COMM_WORLD);
#ifdef COMPARE_PROGRESS
    MPI_Sendrecv(MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);
#endif

    for (int i = 0; i < NUM_COMPUTATIONS; i++) {
      sin(1.0/3.0);
    }

#ifdef COMPARE_PROGRESS
    MPI_Sendrecv(MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);
#endif
    MPI_Barrier(MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
