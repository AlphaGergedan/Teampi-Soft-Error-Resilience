#include <iostream>
#include "Wrapper.h"


/* 
 * This is a simple ping pong MPI code
 */
int main(int argc, char* argv[]) {
	TMPI_Init(&argc, &argv);

	int size;
	TMPI_Comm_size(MPI_COMM_WORLD, &size);

	int rank;
	TMPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (size < 2) {
		std::cout << "2 processors are required" << std::endl;
		return 0;
	}

	int msg;
	if (rank == 0) {
		msg = -1;
		TMPI_Send(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
	}
	else if (rank == 1) {
		TMPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		std::cout << "Process 1 received " << msg << " from process 0\n"; 
	}

	TMPI_Finalize();
}
