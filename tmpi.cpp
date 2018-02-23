#include <stdlib.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <vector>
#include <stddef.h>

#include "tmpi.h"

const int MASTER = 0;

int R_FACTOR;

int world_rank;
int world_size;
int team_rank;
int team_size;

struct Timing {

  enum Method {Send, Recv, Isend, Irecv};

  std::map<const void*,double> sendLog;
  std::map<void*,double> recvLog;
  std::map<void*,double> iSendStartLog;
  std::map<void*,double> iSendEndLog;
  std::map<void*,double> iRecvStartLog;
  std::map<void*,double> iRecvEndLog;

  std::map<void*,double> testLUT;

  double startTime;

  Timing() : startTime(-1) {}

} timer;

std::string getEnvString(std::string const& key)
{
    char const* val = std::getenv(key.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

void read_config() {
  std::string env;

  env = getEnvString("R_FACTOR");
  R_FACTOR = env.empty() ? 2 : std::stoi(env);

}

void print_config(){
  assert(world_size % R_FACTOR == 0);

  MPI_Barrier(MPI_COMM_WORLD);
  double my_time = MPI_Wtime();
  MPI_Barrier(MPI_COMM_WORLD);
  double times[world_size];

  MPI_Gather(&my_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  if (world_rank == MASTER) {
    std::cout << "------------TMPI SETTINGS------------\n";
    std::cout << "R_FACTOR = " << R_FACTOR << "\n";

    std::cout << "Team size: " << team_size << "\n";
    std::cout << "Total ranks: " << world_size << "\n\n";


    if (world_rank == MASTER) {
      for (int i=0; i < world_size; i++) {
        std::cout << "Tshift(" << i << "->" << map_world_to_team(i) << "->" << map_team_to_world(map_world_to_team(i),get_R_number(i)) << ") = " << times[i] - times[0] << "\n";
      }
    }
    std::cout << "--------------------------------------\n\n";
  }

  MPI_Barrier(MPI_COMM_WORLD);
}

void output_timing() {
//  int count = 9;
//  int array_of_blocklengths[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1};
//  MPI_Aint array_of_displacements[] = { offsetof( Timing, numWaits),
//                                        offsetof( Timing, waitTime),
//                                        offsetof( Timing, numBarriers),
//                                        offsetof( Timing, barrierTime),
//                                        offsetof( Timing, numSends),
//                                        offsetof( Timing, sendTime),
//                                        offsetof( Timing, numRecvs),
//                                        offsetof( Timing, recvTime),
//                                        offsetof( Timing, numTests),
//                                      };
//  MPI_Datatype array_of_types[] = { MPI_INT, MPI_DOUBLE, MPI_INT, MPI_DOUBLE, MPI_INT, MPI_DOUBLE, MPI_INT, MPI_DOUBLE, MPI_INT };
//  MPI_Datatype tmp_type, mpi_timer_type;
//  MPI_Aint lb, extent;
//
//  MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, &tmp_type);
//  MPI_Type_get_extent(tmp_type, &lb, &extent);
//  MPI_Type_create_resized( tmp_type, lb, extent, &mpi_timer_type);
//  MPI_Type_commit(&mpi_timer_type);
//
//  Timing timers[world_size];
//  MPI_Gather(&timer, 1, mpi_timer_type, timers, 1, mpi_timer_type, MASTER, MPI_COMM_WORLD);
//
//  if (world_rank == MASTER) {
//    char sep = ',';
//    std::ofstream f;
//    f.open("timings.csv");
//    f << "world_rank,team_rank,replica,numWaits,waitTime,numBarriers,barrierTime,numSends,sendTime,numRecvs,recvTime,numTests\n";
//    for (int rank=0; rank < world_size; rank++) {
//      f << rank << sep
//        << map_world_to_team(rank) << sep
//        << get_R_number(rank) << sep
//        << timers[rank].numWaits << sep
//        << timers[rank].waitTime << sep
//        << timers[rank].numBarriers << sep
//        << timers[rank].barrierTime << sep
//        << timers[rank].numSends << sep
//        << timers[rank].sendTime << sep
//        << timers[rank].numRecvs << sep
//        << timers[rank].recvTime << sep
//        << timers[rank].numTests << sep
//        << std::endl;
//    }
//    f.close();
//  }
  if ((team_rank == 1) && (get_R_number(world_rank) == 0)) {
    int i = 0;
    for (const auto& p : timer.sendLog) {
      i++;
      std::cout << "Send " << i << p.second - timer.startTime << "\n";
    }
  }

}


int init_rank() {
  /**
   * The application should have no knowledge of the world_size or world_rank
   */
  read_config();

  PMPI_Comm_size(MPI_COMM_WORLD, &world_size);
  team_size = (world_size) / R_FACTOR;

  PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  team_rank = map_world_to_team(world_rank);

  print_config();

  int r_num = get_R_number(world_rank);
  assert(world_rank == map_team_to_world(map_world_to_team(world_rank), r_num));


  return MPI_SUCCESS;
}

int get_R_number(int rank) {
  if (rank < R_FACTOR) {
    return rank;
  } else {
    return (rank - R_FACTOR) / (team_size - 1);
  }
}


int map_world_to_team(int rank) {
  if (rank == MPI_ANY_SOURCE) {
    return MPI_ANY_SOURCE;
  } else {
    if (rank < R_FACTOR) {
      return MASTER;
    } else {
      return 1 + ((rank - R_FACTOR) % (team_size - 1));
    }
  }
}


int map_team_to_world(int rank, int r_num) {
  if (rank == MPI_ANY_SOURCE) {
    return MPI_ANY_SOURCE;
  } else {
    if (rank == MASTER) {
      return r_num;
    } else {
      return (rank + R_FACTOR) + (r_num * (team_size -1)) - 1;
    }
  }
}


void remap_status(MPI_Status *status) {
  if (status != MPI_STATUS_IGNORE) {
    //logDebug("remap status source " << status->MPI_SOURCE << " to " << map_world_to_team(status->MPI_SOURCE));
    status->MPI_SOURCE = map_world_to_team(status->MPI_SOURCE);
  }
}

void checkIterationSize(int tag, MPI_Datatype datatype) {
  if (tag == 3) {
    int size;
    MPI_Type_size(datatype, &size);
    assert(size == 29);
  }
}

int MPI_Init(int *argc, char*** argv) {
  PMPI_Init(argc, argv);

  timer.startTime = MPI_Wtime();

  init_rank();

  logDebug("Initialised with size: " << team_size << " / " << world_size);

  return MPI_SUCCESS;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided ) {
  PMPI_Init_thread(argc, argv, required, provided);

  init_rank();
  logDebug("Initialised with size: " << team_size << " / " << world_size);

  return MPI_SUCCESS;
}

int MPI_Is_thread_main(int* flag) {
  // See header documentation
  *flag = get_R_number(world_rank) + 1;
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  assert(comm == MPI_COMM_WORLD);

  *rank = team_rank;
//  logDebug("Returning rank " << *rank);

  return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  assert(comm == MPI_COMM_WORLD);

  *size = team_size;
//  logDebug("Returning size " << *size);

  return MPI_SUCCESS;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Send(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm);

  timer.sendLog.insert(std::make_pair(buf, MPI_Wtime()));

  logDebug("Send to rank " << map_team_to_world(dest, r_num) << " with tag " << tag);

  return err;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Recv(buf, count, datatype, map_team_to_world(source, r_num), tag, comm, status);
  remap_status(status);
  logDebug("Receive from rank " << map_team_to_world(source, r_num) << " with tag " << tag);

  return err;
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  err |= PMPI_Isend(buf, count, datatype, map_team_to_world(dest, r_num), tag, comm, request);
  logDebug("Isend to rank " << map_team_to_world(dest, r_num) << " with tag " << tag);

  return err;
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
               MPI_Comm comm, MPI_Request *request) {
  assert(comm == MPI_COMM_WORLD);

  int err = 0;

  int r_num = get_R_number(world_rank);
  source = map_team_to_world(source, r_num);
  err |= PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
  logDebug("Ireceive from rank " << map_team_to_world(source, r_num) << " with tag " << tag);

  return err;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  int err = 0;
  logDebug("Wait initialised");

  err |= PMPI_Wait(request, status);

  remap_status(status);
  logDebug("Wait completed "
      <<"(STATUS_SOURCE=" << status->MPI_SOURCE
      << ",STATUS_TAG=" << status->MPI_TAG
      << ")");

  return err;
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  int err = 0;

  err |= PMPI_Test(request, flag, status);
  remap_status(status);

  logDebug("Test completed ("
      << "FLAG=" << *flag
      << ",STATUS_SOURCE=" << status->MPI_SOURCE
      << ",STATUS_TAG=" << status->MPI_TAG
      << ")");
  return err;
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  assert(comm == MPI_COMM_WORLD);
  int err = 0;

  logDebug(
      "Probe initialised (SOURCE="
      << source
      << ",TAG="
      << tag
      << ")");
  int r_num = get_R_number(world_rank);
  err |= PMPI_Probe(map_team_to_world(source, r_num), tag, comm, status);
  remap_status(status);
  logDebug(
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

  int r_num = get_R_number(world_rank);
  err |= PMPI_Iprobe(map_team_to_world(source, r_num), tag, comm, flag, status);
  remap_status(status);
  logDebug(
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

  err |= PMPI_Barrier(comm);

  return err;
}

int MPI_Finalize() {
  logDebug("Finalize");
  output_timing();

  return PMPI_Finalize();
}
