#include "UnitTests.h"

#include <iostream>
#include <locale>
#include <cassert>

#define IS_REAL_MASTER  ((rank == 0) && (r_num == 1))

void helloWorldTest(int rank, int size) {
  char proc_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(proc_name, &name_len);

  std::cout << "Hello world from processor " << proc_name << ", rank " << rank << " out of " << size << "\n";
}

void pingPongTest(int rank, int size) {
  int msg;
  if (rank == 0) {
    msg = -1;
    MPI_Send(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
  }
  else if (rank == 1) {
    MPI_Status status;
    MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

    assert(status.MPI_SOURCE == 0);
    assert(status.MPI_TAG == 0);
    assert(msg == -1);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 1) {
    msg = 10;
    MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  } else if (rank == 0) {
    MPI_Status status;
    MPI_Recv(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

    assert(status.MPI_SOURCE == 1);
    assert(status.MPI_TAG == 0);
    assert(msg == 10);
  }
}

void nonBlockingTest(int rank, int size) {
  MPI_Request* request = new MPI_Request();
  MPI_Status status;
  int flag;
  int msg;
  if (rank == 0) {
    msg = -1;
    MPI_Isend(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, request);
    MPI_Test(request, &flag, &status);
    if (!flag) {
      MPI_Wait(request, &status);
    }
  }
  else if (rank == 1) {
    MPI_Irecv(&msg, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, request);
    MPI_Test(request, &flag, &status);
    if (!flag) {
      MPI_Wait(request, &status);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
//  if (rank == 1) {
//    msg = 10;
//    MPI_Request* request = new MPI_Request();
//    MPI_Isend(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, request);
//    MPI_Wait(request, MPI_STATUS_IGNORE);
//  } else if (rank == 0) {
//    MPI_Request* request = new MPI_Request();
//    MPI_Irecv(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, request);
//    MPI_Wait(request, MPI_STATUS_IGNORE);
//    std::cout << "Process 0 received " << msg << " from process 1\n";
//  }
}


void runTest(std::string testName, std::function<void(int,int)> test, int rank, int size) {
  MPI_Barrier(MPI_COMM_WORLD);
  int r_num;
  MPI_Is_thread_main(&r_num);
  if (IS_REAL_MASTER)
    std::cout << "<<< STARTING TEST: " << testName << " >>>" << std::endl;
  MPI_Barrier(MPI_COMM_WORLD);
  test(rank, size);

  MPI_Barrier(MPI_COMM_WORLD);
  if (IS_REAL_MASTER)
    std::cout << "### TEST FINISHED: " << testName << " ###" << std::endl << std::endl;
  MPI_Barrier(MPI_COMM_WORLD);
}


int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int rank;
  int size;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  runTest("Hello World", helloWorldTest, rank, size);

  runTest("Ping Pong", pingPongTest, rank, size);

  runTest("Non Blocking", nonBlockingTest, rank, size);


  MPI_Finalize();
}
