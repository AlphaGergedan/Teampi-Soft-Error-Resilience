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

	char proc_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(proc_name, &name_len);

	std::cout << "Hello world from processor " << proc_name << ", rank " << rank << " out of " << size << "\n";

	MPI_Finalize();
}
