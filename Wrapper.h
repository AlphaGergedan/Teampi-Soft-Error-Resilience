#include <mpi.h>
#include <vector>

int get_R_number();
int map_world_to_team(int world_rank);
std::vector<int> map_team_to_world(int rank);


int TMPI_Init(int *argc, char*** argv);

int TMPI_Comm_rank(MPI_Comm comm, int *rank);

int TMPI_Comm_size(MPI_Comm comm, int *size);

int TMPI_Send(const void *buf, int count, MPI_Datatype, int dest, int tag, MPI_Comm comm);

int TMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
 			  MPI_Comm comm, MPI_Status *status);

int TMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request);

int TMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Request *request);

int TMPI_Wait(MPI_Request *request, MPI_Status *status);

int TMPI_Finalize(void);
