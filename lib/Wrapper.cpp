#include "Wrapper.h"
#include "RankOperations.h"
#include "TMPIConstants.h"
#include "Logging.h"
#include "Timing.h"

int MPI_Init(int *argc, char*** argv) {
  PMPI_Init(argc, argv);

  init_rank();

  return MPI_SUCCESS;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ) {
  PMPI_Init_thread(argc, argv, required, provided);

  init_rank();

  return MPI_SUCCESS;
}

int MPI_Is_thread_main(int* flag) {
  // See header documentation
  *flag = get_R_number(getWorldRank()) + 1;
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  assert(comm == MPI_COMM_WORLD);

  *rank = getTeamRank();
//  logInfo("Returning rank " << *rank);

  return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  assert(comm == MPI_COMM_WORLD);

  *size = getTeamSize();
//  logInfo("Returning size " << *size);

  return MPI_SUCCESS;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(getWorldRank());
  err |= PMPI_Send(buf, count, datatype, dest, tag, getCommunicator());

  logInfo(
      "Send to rank " <<
      dest <<
      "/" <<
      map_team_to_world(dest, r_num) <<
      " with tag " <<
      tag);

  return err;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(getWorldRank());
  err |= PMPI_Recv(buf, count, datatype, source, tag, getCommunicator(), status);

  remap_status(status);

  logInfo(
      "Receive from rank " <<
      source <<
      "/" <<
      map_team_to_world(source, r_num) <<
      " with tag " <<
      tag);

  return err;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(getWorldRank());
  err |= PMPI_Isend(buf, count, datatype, dest, tag, getCommunicator(), request);
  Timing::startNonBlocking(Timing::NonBlockingType::iSend, tag, request);

  logInfo(
      "Isend to rank " <<
      dest <<
      "/" <<
      map_team_to_world(dest, r_num) <<
      " with tag " <<
      tag);

  return err;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(getWorldRank());

  err |= PMPI_Irecv(buf, count, datatype, source, tag, getCommunicator(), request);

  Timing::startNonBlocking(Timing::NonBlockingType::iRecv, tag, request);

  logInfo(
      "Receive from rank " <<
      source <<
      "/" <<
      map_team_to_world(source, r_num) <<
      " with tag " <<
      tag);

  return err;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  int err = 0;
  logInfo("Wait initialised");

  err |= PMPI_Wait(request, status);

  Timing::endNonBlocking(request, status);

  remap_status(status);
  logInfo("Wait completed "
      <<"(STATUS_SOURCE=" << status->MPI_SOURCE
      << ",STATUS_TAG=" << status->MPI_TAG
      << ")");

  return err;
}

int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) {
  int err = 0;

  logInfo("Waitall initialised with " << count << " requests");

  err |= PMPI_Waitall(count, array_of_requests, array_of_statuses);

  for (int i = 0; i < count; i++) {
    remap_status(&array_of_statuses[i]);
  }

  logInfo("Waitall completed with " << count << " requests");

  return err;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  int err = 0;

  err |= PMPI_Test(request, flag, status);

  if (*flag) {
    Timing::endNonBlocking(request, status);
    remap_status(status);
  }

  logInfo("Test completed ("
      << "FLAG=" << *flag
      << ",STATUS_SOURCE=" << status->MPI_SOURCE
      << ",STATUS_TAG=" << status->MPI_TAG
      << ")");
  return err;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);
  int err = 0;

  logInfo(
      "Probe initialised (SOURCE="
      << source
      << ",TAG="
      << tag
      << ")");
  int r_num = get_R_number(getWorldRank());
  err |= PMPI_Probe(source, tag, getCommunicator(), status);
  remap_status(status);
  logInfo(
      "Probe finished ("
      << "SOURCE=" << map_team_to_world(source, r_num)
      << ",TAG=" << tag
      << ",STATUS_SOURCE=" << status->MPI_SOURCE
      << ",STATUS_TAG=" << status->MPI_TAG
      << ")");

  return err;

}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
                MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(getWorldRank());
  err |= PMPI_Iprobe(source, tag, getCommunicator(), flag, status);
  remap_status(status);
  logInfo(
      "Iprobe finished ("
      << "FLAG=" << *flag
      << ",SOURCE=" << map_team_to_world(source, r_num)
      << ",TAG=" << tag
      << ",STATUS_SOURCE=" << status->MPI_SOURCE
      << ",STATUS_TAG=" << status->MPI_TAG
      << ")");

  return err;
}

int MPI_Barrier(MPI_Comm comm) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  err |= PMPI_Barrier(getCommunicator());

  return err;
}

int MPI_Finalize() {
  logInfo("Start finalize");

  freeCommunicator();
  //output_timing();

  return PMPI_Finalize();
//  logInfo("End finalize");
//
//  return 0;
}
