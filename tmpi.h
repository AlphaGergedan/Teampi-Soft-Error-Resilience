#ifndef TMPI_H
#define TMPI_H

#include <mpi.h>


enum class CommunicationModes { Parallel, Mirror };

enum class ReplicationModes { Cyclic, Adjacent };

std::string getEnvString(std::string const& key);

void read_config();

void print_config();

/**
 * Sets the global variables for an MPI process
 * @return MPI_SUCCESS
 */
int init_rank();

/**
 * @param rank of process
 * @return which replica rank is [0..R_FACTOR]
 */
int get_R_number(int rank);

/**
 * @param world_rank
 * @return world_rank mapped to a team_rank
 */
int map_world_to_team(int world_rank);

/**
 *
 * @param rank of a replica
 * @param r_num specifying which replica rank is
 * @return rank mapped to a world/global rank
 */
int map_team_to_world(int rank, int r_num);

/**
 * Modify the source of the status to within team_size
 * @param status to modify
 */
void remap_status(MPI_Status *status);


/**
 * Sets up the MPI library and initialises the process
 * @param argc
 * @param argv
 * @return
 */
int MPI_Init(int *argc, char*** argv);

/**
 * @see MPI_Init
 * @param argc
 * @param argv
 * @param required
 * @param provided
 * @return
 */
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided);

/**
 *
 * @param comm
 * @param rank is set to team_rank
 * @return
 */
int MPI_Comm_rank(MPI_Comm comm, int *rank);


/**
 *
 * @param comm
 * @param size set to team_size
 * @return
 */
int MPI_Comm_size(MPI_Comm comm, int *size);


/**
 * Sends message to dest + all replicas in mirror mode or host rank == master
 * Sends message to dest in parallel mode
 * Sends to master only if dest==master
 * @return
 */
int MPI_Send(const void *buf, int count, MPI_Datatype, int dest, int tag,
              MPI_Comm comm);


/**
 * Recv from master only if source is master
 * Recv from source + all replicas in mirror mode
 * Recv from source in parallel mode
 * @return
 */
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status);


/**
 * @see MPI_Send
 * @return
 */
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request);

/**
 * @see MPI_Recv
 * @return
 */
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request);

/**
 * @param request an array in mirror mode (per replica), single request object in parallel
 * @param status an array in mirror mode (per replica), single status object in parallel
 * @return
 */
int MPI_Wait(MPI_Request *request, MPI_Status *status);

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status);

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);

int MPI_Finalize(void);
#endif
