#include "Wrapper.h"

/* Set the replication factor */
#define R_FACTOR 2

int world_rank;
int world_size;
int team_rank;
int team_size;

int TMPI_Init(int *argc, char*** argv) {
	MPI_Init(argc, argv);

	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	team_size = world_size / R_FACTOR;

	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	team_rank = world_rank % team_size;

	return MPI_SUCCESS;
}

int TMPI_Comm_rank(MPI_Comm comm, int *rank) {
	*rank = team_rank;
	return MPI_SUCCESS;
}

int TMPI_Comm_size(MPI_Comm comm, int *size) {
	*size = team_size;
	return MPI_SUCCESS;
}

int TMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	if (world_rank < team_size) {
		for (int i=0; i < R_FACTOR; i++) {
			MPI_Send(buf, count, datatype, dest+(i*team_size), tag, comm);	
		}
	}
	return MPI_SUCCESS;
}

int TMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, 
			  MPI_Comm comm, MPI_Status *status) {
	MPI_Recv(buf, count, datatype, source, tag, comm, status);
}
