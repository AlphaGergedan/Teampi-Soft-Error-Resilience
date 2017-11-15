#include <stdlib.h>
#include "Wrapper.h"


// Set the replication factor
#define R_FACTOR 2


// Communication strategies
#define COMM_MODE PARALLEL

// Replication Strategies {CYCLIC, ADJACENT}
#define REP_MODE CYCLIC
#define REPLICATE_MASTER 1

int world_rank;
int world_size;
int team_rank;
int team_size;

int get_R_number()
{
	if (REPLICATE_MASTER) {
		if (REP_MODE == CYCLIC) {
			return world_rank / team_size;
		}
		else if (REP_MODE == ADJACENT) {
			return world_rank % R_FACTOR;
		}
	}
	else {
		if (REP_MODE == CYCLIC) {
			return (world_rank - 1) / team_size;
		}
		else if (REP_MODE == ADJACENT) {
			return (world_rank - 1) % R_FACTOR;
		}
	}
}

int map_world_to_team(int rank)
{

	if (REPLICATE_MASTER) {
		if (REP_MODE == CYCLIC) {
			return ((rank - 1) % team_size) + 1;
		}
		else if (REP_MODE == ADJACENT) {
			return (rank + R_FACTOR - 1) / R_FACTOR;
		} else {
			assert(false);
		}
	}
	else {
		if (REP_MODE == CYCLIC) {
			return rank % team_size;
		}
		else if (REP_MODE == ADJACENT) {
			return rank / R_FACTOR;
		} else {
			assert(false);
		}
	}
}

std::vector<int> map_team_to_world(int rank)
{

}



int TMPI_Init(int *argc, char*** argv) {
	MPI_Init(argc, argv);
	
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	team_size = world_size / R_FACTOR;

	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	team_rank = map_world_to_team(world_rank);

	return MPI_SUCCESS;
}

int TMPI_Comm_rank(MPI_Comm comm, int *rank) {
	assert(comm == MPI_COMM_WORLD);

	*rank = team_rank;

	return MPI_SUCCESS;
}

int TMPI_Comm_size(MPI_Comm comm, int *size) {
	assert(comm == MPI_COMM_WORLD);

	*size = team_size;

	return MPI_SUCCESS;
}

int TMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	assert(comm == MPI_COMM_WORLD);

	if (COMM_MODE == PARALLEL) {
		// TODO think about ways to implement this --> team array with world ranks in?
		MPI_Send(buf, count, datatype, dest+(i*team_size), tag, comm);
	}

	if (world_rank < team_size) {
		for (int i=0; i < R_FACTOR; i++) {
			MPI_Send(buf, count, datatype, dest+(i*team_size), tag, comm);
		}
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
