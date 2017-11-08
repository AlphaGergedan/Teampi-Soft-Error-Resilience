#include <stdlib.h>
#include "Wrapper.h"


/* Set the replication factor */
#define R_FACTOR 2

#define REPLICATE_MASTER 1

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
	if (comm == MPI_COMM_WORLD) {
		*rank = team_rank;
	} 
	else {	
		MPI_Comm_rank(comm, rank);
	}
	return MPI_SUCCESS;
}

int TMPI_Comm_size(MPI_Comm comm, int *size) {
	if (comm == MPI_COMM_WORLD) {
		*size = team_size;
	} 
	else {
		MPI_Comm_size(comm, size);
	}
	return MPI_SUCCESS;
}

int TMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	if (comm == MPI_COMM_WORLD) {
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

int TMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request)
{
	request = (MPI_Request *)realloc(request, R_FACTOR * sizeof(MPI_Request));

	MPI_Isend(buf, count, datatype, dest, tag, comm, &request[0]);

	for (int i=1; i < R_FACTOR; i++) {
		MPI_Isend(buf, count, datatype, dest+(i*team_size), tag, comm, &request[i]);
	}
	return MPI_SUCCESS;
}

int TMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Request *request)
{
	return MPI_Irecv(buf, count, datatype, source, tag, comm, &request[world_rank / team_size]);
}

int TMPI_Wait(MPI_Request *request, MPI_Status *status)
{
	return MPI_Wait(&request[0], status);
}

int TMPI_Finalize() {
	return MPI_Finalize();
}
