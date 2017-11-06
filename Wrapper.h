#include <mpi.h>

int TMPI_Init(int *argc, char*** argv);

int TMPI_Comm_rank(MPI_Comm comm, int *rank);

int TMPI_Comm_size(MPI_Comm comm, int *size);

int TMPI_Send(const void *buf, int count, MPI_Datatype, int dest, int tag, MPI_Comm comm);

int TMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
 			  MPI_Comm comm, MPI_Status *status);

int TMPI_Finalize(void);