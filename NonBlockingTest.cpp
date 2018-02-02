#include <iostream>

#include <mpi.h>


/* 
 * This is a simple ping pong MPI code
 */
int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (size != 2) {
		std::cout << "2 processors are required" << std::endl;
		return 0;
	}

  MPI_Request* request = new MPI_Request();
  MPI_Status status;
  int flag;
	int msg;
	if (rank == 0) {
		msg = -1;
		MPI_Isend(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, request);
		MPI_Test(request, &flag, &status);
//		std::cout << "Flag returned " << flag << "\n";
//		std::cout << "Status source after test: " << status.MPI_SOURCE << "\n";
		MPI_Wait(request, &status);
//    std::cout << "Process 0 sent " << msg << " to process 1\n";
//    std::cout << "Status source after wait: " << status.MPI_SOURCE << "\n";
	}
	else if (rank == 1) {
		MPI_Irecv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, request);
    MPI_Test(request, &flag, &status);
//    std::cout << "Flag returned " << flag << "\n";
//    std::cout << "Status source after test: " << status.MPI_SOURCE << "\n";
    MPI_Wait(request, &status);
		std::cout << "Process 1 received " << msg << " from process 0\n";
//    std::cout << "Status source after wait: " << status.MPI_SOURCE << "\n";
	}

	MPI_Barrier(MPI_COMM_WORLD);
//	if (rank == 1) {
//	  msg = 10;
//    MPI_Request* request = new MPI_Request();
//	  MPI_Isend(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, request);
//    MPI_Wait(request, MPI_STATUS_IGNORE);
//	} else if (rank == 0) {
//    MPI_Request* request = new MPI_Request();
//	  MPI_Irecv(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, request);
//    MPI_Wait(request, MPI_STATUS_IGNORE);
//    std::cout << "Process 0 received " << msg << " from process 1\n";
//	}

	MPI_Finalize();
}
