#include "Wrapper.h"

#include <cassert>
#include <unistd.h>

#include "Logging.h"
#include "Rank.h"
#include "Timing.h"

int MPI_Init(int *argc, char*** argv) {
  int err = PMPI_Init(argc, argv);
  initialiseTMPI();
  return err;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ) {
  int err = PMPI_Init_thread(argc, argv, required, provided);
  initialiseTMPI();
  return err;
}

int MPI_Is_thread_main(int* flag) {
  // See header documentation
  *flag = getTeam();
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  // todo: assert that a team comm is used
  //assert(comm == MPI_COMM_WORLD);
  if(comm==MPI_COMM_WORLD) 
   *rank = getTeamRank();
  else 
   PMPI_Comm_rank(comm, rank);
  return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  //assert(comm == MPI_COMM_WORLD);
  *size = getTeamSize();
  return MPI_SUCCESS;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  int err = PMPI_Comm_dup(getTeamComm(comm), newcomm);
  logInfo("Created communicator " << *newcomm);
  assert(err==MPI_SUCCESS);
  return err;
}

int MPI_Comm_free(MPI_Comm *comm) {
  assert(*comm != MPI_COMM_WORLD);
  logInfo("Free communicator " << *comm);
  int err = PMPI_Comm_free(comm);
  assert(err==MPI_SUCCESS);
  return err;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Send(buf, count, datatype, dest, tag, getTeamComm(comm));
  logInfo("Send to rank " << dest << "/" << mapTeamToWorldRank(dest) << " with tag " << tag);
  return err;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Recv(buf, count, datatype, source, tag, getTeamComm(comm), status);
  logInfo("Receive from rank " << source << "/" << mapTeamToWorldRank(source) << " with tag " << tag);
  return err;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm) {
  int err = PMPI_Allgather(sendbuf, sendcount, sendtype,
                           recvbuf, recvcount, recvtype,
                           getTeamComm(comm)); 
  return err;
}

int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request * request) {
  int err = PMPI_Iallgather(sendbuf, sendcount, sendtype,
                            recvbuf, recvcount, recvtype,
                            getTeamComm(comm), request);
  return err;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Isend(buf, count, datatype, dest, tag, getTeamComm(comm), request);
  logInfo("Isend to rank " << dest << "/" << mapTeamToWorldRank(dest) << " with tag " << tag);
  return err;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Irecv(buf, count, datatype, source, tag, getTeamComm(comm), request);
  logInfo("Receive from rank " << source << "/" << mapTeamToWorldRank(source) << " with tag " << tag);
  return err;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  logInfo("Wait initialised");
  int err = PMPI_Wait(request, status);
  logInfo("Wait completed");
  return err;
}

int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) {
  logInfo("Waitall initialised with " << count << " requests");
  int err = PMPI_Waitall(count, array_of_requests, array_of_statuses);
  logInfo("Waitall completed with " << count << " requests");
  return err;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  int err = PMPI_Test(request, flag, status);
  logInfo("Test completed (FLAG=" << *flag << ",STATUS_SOURCE=" << status->MPI_SOURCE << ",STATUS_TAG=" << status->MPI_TAG << ")");
  return err;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  //assert(comm == MPI_COMM_WORLD);
  logInfo("Probe initialised (SOURCE=" << source << ",TAG=" << tag << ")");
  int err = PMPI_Probe(source, tag, getTeamComm(comm), status);
  logInfo("Probe finished (SOURCE=" << mapTeamToWorldRank(source) << ",TAG=" << tag << ",STATUS_SOURCE=" << status->MPI_SOURCE << ",STATUS_TAG=" << status->MPI_TAG << ")");
  return err;
}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Iprobe(source, tag, getTeamComm(comm), flag, status);
  logInfo("Iprobe finished (FLAG=" << *flag << ",SOURCE=" << mapTeamToWorldRank(source) << ",TAG=" << tag << ",STATUS_SOURCE=" << status->MPI_SOURCE << ",STATUS_TAG=" << status->MPI_TAG << ")");
  return err;
}

int MPI_Barrier(MPI_Comm comm) {
  //assert(comm == MPI_COMM_WORLD);
  int err = synchroniseRanksInTeam();
  return err;
}

int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm ) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Bcast(buffer, count, datatype, root, getTeamComm(comm));
  return err;
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, getTeamComm(comm));
  return err;
}

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, getTeamComm(comm));
  return err;
}

int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                  MPI_Comm comm) {
  //assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, getTeamComm(comm));
  return err;
}

double MPI_Wtime() {
  // If you mark on the timeline here expect negative time values (you've been warned)
  return PMPI_Wtime();
}

int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                int dest, int sendtag,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int source, int recvtag,
                MPI_Comm comm, MPI_Status *status) {
  if (comm == MPI_COMM_SELF) {
    if (sendcount == 0) {
      Timing::markTimeline(sendtag);
    } else {
      Timing::markTimeline(sendtag, sendbuf, sendcount, sendtype);
    }
  } else {
    //assert(comm == MPI_COMM_WORLD);
    MPI_Sendrecv(sendbuf, sendcount, sendtype,dest,sendtag,recvbuf,recvcount,recvtype,source,recvtag, getTeamComm(comm),status);
  }
  return MPI_SUCCESS;
}

int MPI_Finalize() {
  logInfo("Finalize");
  Timing::finaliseTiming();
  // Wait for all replicas before finalising
  PMPI_Barrier(MPI_COMM_WORLD);
  freeTeamComm();
  Timing::outputTiming();
  return PMPI_Finalize();
}

int MPI_Abort(MPI_Comm comm, int errorcode) {
  assert(comm == MPI_COMM_WORLD);
  int err = PMPI_Abort(getTeamComm(comm), errorcode);
  return err;
}
