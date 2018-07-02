/*
 * Latency.cpp
 *
 *  Created on: 5 Jun 2018
 *      Author: Ben Hazelwood
 */

#include <mpi.h>
#include <iostream>
#include <assert.h>
#include <cstdlib>

#define COMPARE_HASH


const int NUM_TRIALS = 1000;
const int NUM_PONGS  = 1e4;

int START = 0;
int FINISH = 0;
int INCR = 0;

int NUM_POINTS = 0;

const int MAX_POINTS = 1e4;

int main(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  if (argc != 4) {
    std::cout << "Usage: mpirun Latency <START> <FINISH> <INCREMENT>\n";
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  START = std::stoi(argv[1]);
  FINISH = std::stoi(argv[2]);
  INCR = std::stoi(argv[3]);

  NUM_POINTS = (FINISH - START) / INCR;


  assert(NUM_POINTS < MAX_POINTS);

  int rank;
  int size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  const int source = 0;
  const int dest   = size - 1;

  char m[FINISH];

  if (rank == source) {
    for (int i=0; i < FINISH; i++) {
      m[i] = '0';
    }
  }

  double t1 = 0.0;
  double t2 = 0.0;

  double bandwidth[NUM_POINTS];

  MPI_Status status;
  int counter;
  for (int t = 0; t < NUM_TRIALS; t++) {
#ifdef COMPARE_PROGRESS
    MPI_Sendrecv(MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);
#endif
    MPI_Barrier(MPI_COMM_WORLD);
    counter = 0;
    for (int n = START; n <= FINISH; n += INCR) {
      if (rank == source) {
        // Handshake
//        MPI_Recv(&m, 1, MPI_CHAR, dest, 0, MPI_COMM_WORLD, &status);
        t1 = MPI_Wtime();

        for (int p = 0; p < NUM_PONGS; p++) {
          MPI_Send(&m, n, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
          MPI_Recv(&m, n, MPI_CHAR, dest, 1, MPI_COMM_WORLD,  &status);
        }

        t2 = MPI_Wtime();
        double b = (sizeof(char) * n * 2.0 * (double)NUM_PONGS) / (t2 - t1);
        if (bandwidth[counter] < b) {
          bandwidth[counter] = b;
        }
      }

      if (rank == dest) {
        // Handshake
//        MPI_Send(&m, 1, MPI_CHAR, source, 0, MPI_COMM_WORLD);
        for (int p = 0; p < NUM_PONGS; p++) {
          MPI_Recv(&m, n, MPI_CHAR, source, 0, MPI_COMM_WORLD,  &status);
          MPI_Send(&m, n, MPI_CHAR, source, 1, MPI_COMM_WORLD);
        }
      }
      counter++;
    }
#ifdef COMPARE_HASH
//    if (rank == source) {
    MPI_Sendrecv(&m, 1, MPI_CHAR, MPI_PROC_NULL, 0, MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);
//    }
#endif
  }


  if (rank == source) {
    counter = 0;
    for (int n=START; n < FINISH; n+=INCR) {
      std::cout << "bandwidth[" << n << "]=" << bandwidth[counter] << "\n";
      counter++;
    }
  }


  MPI_Finalize();

return 0;
}


