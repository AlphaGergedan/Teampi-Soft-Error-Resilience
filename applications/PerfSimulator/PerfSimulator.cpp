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

  for (int t = 0; t < NUM_TRIALS; t++)
  {
    MPI_Barrier(MPI_COMM_WORLD);
#ifdef COMPARE_PROGRESS
    MPI_Sendrecv(MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, -1, MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);
#endif

    for (int i = 0; i < NUM_COMPUTATIONS; i++) {
      sin(1.0/3.0); // Arbitrary computation (the compiler shouldn't optimise it out...)
    }

#ifdef COMPARE_PROGRESS
    MPI_Sendrecv(MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);
#endif
    MPI_Barrier(MPI_COMM_WORLD);
  }

  MPI_Finalize();
  return 0;
}
