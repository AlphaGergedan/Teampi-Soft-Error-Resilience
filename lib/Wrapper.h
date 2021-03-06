#ifndef WRAPPER_H
#define WRAPPER_H

#include <mpi.h>

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
 * IMPORTANT: We use this as a debugging feature to return the replica number
 */
int MPI_Is_thread_main(int *flag);

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
 *
 * @param comm
 * @param newcomm
 * @return
 */
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);

/**
 *
 * @param comm
 * @return
 */ 
int MPI_Comm_free(MPI_Comm *comm);

/**
 * Sends only to the corresponding replica of dest
 * @return
 */
int MPI_Send(const void *buf, int count, MPI_Datatype, int dest, int tag,
              MPI_Comm comm);


/**
 * Receive from the corresponding replica of source
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

int MPI_Wait(MPI_Request *request, MPI_Status *status);

int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status);

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm);

int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request *request);

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);

int MPI_Barrier(MPI_Comm comm);

int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm );

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm);

int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm);

double MPI_Wtime();

/* This is the plugin for the heartbeat called by the application (MPI_COMM_SELF must be used) */
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                int dest, int sendtag,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int source, int recvtag,
                MPI_Comm comm, MPI_Status *status);

int MPI_Finalize(void);

int MPI_Abort(MPI_Comm comm, int errorcode);
#endif
