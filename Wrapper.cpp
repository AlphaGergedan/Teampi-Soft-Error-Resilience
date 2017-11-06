#include "Wrapper.h"

/* Set the replication factor */
#define R_FACTOR 2

int TMPI_Init(int *argc, char*** argv) {
	return MPI_Init(argc, argv);
}

int TMPI_Comm_rank(MPI_Comm comm, int *rank) {
	if (comm == MPI_COMM_WORLD) {
		int team_size;
		TMPI_Comm_size(MPI_COMM_WORLD, &team_size);

		int world_rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
		*rank = world_rank % team_size;
	} 
	else {	
		MPI_Comm_rank(comm, rank);
	}
	return MPI_SUCCESS;
}

int TMPI_Comm_size(MPI_Comm comm, int *size) {
	if (comm == MPI_COMM_WORLD) {
		int world_size;
		MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		*size = world_size / R_FACTOR;
	} 
	else {
		MPI_Comm_size(comm, size);
	}
	return MPI_SUCCESS;
}

int TMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	if (comm == MPI_COMM_WORLD) {
		int team_size;
		TMPI_Comm_size(MPI_COMM_WORLD, &team_size);
		int world_rank;
		TMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

		if (world_rank < team_size) {
			for (int i=0; i < R_FACTOR; i++) {
				MPI_Send(buf, count, datatype, dest+(i*team_size), tag, comm);	
			}
		}
	} 
	else {
		MPI_Send(buf, count, datatype, dest, tag, comm);
	}

	return MPI_SUCCESS;
}

int TMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, 
			  MPI_Comm comm, MPI_Status *status) {
	return MPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int TMPI_Finalize() {
	return MPI_Finalize();
}