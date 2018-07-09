/*
 * Timing.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#include "Timing.h"
#include "Logging.h"
#include "Rank.h"
#include "RankControl.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <stddef.h>
#include <bitset>
#include <unistd.h>

struct Timer {
  // PMPI_Wtime at start of execution
  double startTime;
  // PMPI_Wtime at the end of this ranks execution
  double endTime;

  // Mark when an application sleeps
  std::vector<double> sleepPoints;

  // TODO change to heartbeat terminology
  // Times for each heartbeat (per replica)
  std::map< int, std::vector<double> > heartbeatTimes;
  // Store the MPI_Requests for each heartbeat (per replica) for calling MPI_Test
  std::map< int, std::vector<MPI_Request> > heartbeatTimeRequests;

  // Hash for each heartbeat buffer (per replica)
  std::map<int, std::vector<std::size_t> > heartbeatHashes;
  // Store the MPI_Requests for each heartbeat (per replica) for calling MPI_Test
  std::map<int, std::vector<MPI_Request> > heartbeatHashRequests;
} timer;

void Timing::initialiseTiming() {
  synchroniseRanksInTeam();
  timer.startTime = PMPI_Wtime();
  for (int i=0; i < getNumberOfTeams(); i++) {
    timer.heartbeatTimes.insert(std::make_pair(i,std::vector<double>()));
    timer.heartbeatTimeRequests.insert(std::make_pair(i,std::vector<MPI_Request>()));

    timer.heartbeatHashes.insert(std::make_pair(i,std::vector<std::size_t>()));
    timer.heartbeatHashRequests.insert(std::make_pair(i,std::vector<MPI_Request>()));
  }
}

void Timing::finaliseTiming() {
  synchroniseRanksInTeam();
  timer.endTime = PMPI_Wtime();
}

void Timing::markTimeline() {
    timer.heartbeatTimes.at(getTeam()).push_back(PMPI_Wtime());
    compareProgressWithReplicas();
}

void Timing::markTimeline(const void *sendbuf, int sendcount, MPI_Datatype sendtype) {
  markTimeline();
  compareBufferWithReplicas(sendbuf, sendcount, sendtype);
}

void Timing::compareProgressWithReplicas() {
  for (int r=0; r < getNumberOfTeams(); r++) {
    if (r != getTeam()) {
      // Send out this replica's times
      MPI_Request request;
      PMPI_Isend(&timer.heartbeatTimes.at(getTeam()).back(), 1, MPI_DOUBLE,
                mapTeamToWorldRank(getTeamRank(), r), getTeam(),
                getLibComm(), &request);
      MPI_Request_free(&request);

      // Receive times from other replicas
      timer.heartbeatTimes.at(r).push_back(0.0);
      timer.heartbeatTimeRequests.at(r).push_back(MPI_Request());
      PMPI_Irecv(&timer.heartbeatTimes.at(r).back(), 1, MPI_DOUBLE,
                 mapTeamToWorldRank(getTeamRank(), r), r, getLibComm(), &timer.heartbeatTimeRequests.at(r).back());

      // Test for completion of Irecv's
      int numPending = 0;
      for (int i=0; i < timer.heartbeatTimeRequests.at(r).size(); i++) {
        int flag = 0;
        PMPI_Test(&timer.heartbeatTimeRequests.at(r).at(i), &flag, MPI_STATUS_IGNORE);
        numPending += 1 - flag;
      }
    }
  }
}

void Timing::compareBufferWithReplicas(const void *sendbuf, int sendcount, MPI_Datatype sendtype) {
  if (getShouldCorruptData()) {
    //TODO can remove const here via cast (assuming data was originally non-const) and corrupt properly, no need for now
    sendcount++; // This isn't really that safe either...likely causes memory corruption occasionally
    setShouldCorruptData(false);
  }

  int typeSize;
  MPI_Type_size(sendtype, &typeSize);

  std::string bits((const char*)sendbuf, sendcount*typeSize);
  std::hash<std::string> hash_fn;
  std::size_t hash = hash_fn(bits);
  timer.heartbeatHashes.at(getTeam()).push_back((std::size_t)hash);

  for (int r=0; r < getNumberOfTeams(); r++) {
    if (r != getTeam()) {
      // Send out this replica's times
      MPI_Request request;
      PMPI_Isend(&timer.heartbeatHashes.at(getTeam()).back(), 1, TMPI_SIZE_T,
                mapTeamToWorldRank(getTeamRank(), r), getTeam(),
                getLibComm(), &request);
      MPI_Request_free(&request);

      // Receive times from other replicas
      timer.heartbeatHashes.at(r).push_back(0);
      timer.heartbeatHashRequests.at(r).push_back(MPI_Request());
      PMPI_Irecv(&timer.heartbeatHashes.at(r).back(), 1, TMPI_SIZE_T,
                 mapTeamToWorldRank(getTeamRank(), r), r, getLibComm(), &timer.heartbeatHashRequests.at(r).back());

      // Test for completion of Irecv's
      int numPending = 0;
      for (int i=0; i < timer.heartbeatHashRequests.at(r).size(); i++) {
        int flag = 0;
        PMPI_Test(&timer.heartbeatHashRequests.at(r).at(i), &flag, MPI_STATUS_IGNORE);
        numPending += 1 - flag;
      }
      std::cout << "Num pending: " << numPending << "\n";
    }
  }
}

void Timing::sleepRankRaised() {
  timer.sleepPoints.push_back(PMPI_Wtime());
}


void Timing::outputTiming() {
  std::cout.flush();
  PMPI_Barrier(MPI_COMM_WORLD);

  std::string filenamePrefix = getEnvString("TMPI_FILE");

  // Output simple replica timings
  if ((getTeamRank() == MASTER) && (getWorldRank() != MASTER)) {
    PMPI_Send(&timer.endTime, 1, MPI_DOUBLE, MASTER, 0, getLibComm());
  }


  if (getWorldRank() == MASTER) {
    std::cout << std::endl;
    std::cout << "----------TMPI_TIMING----------\n";
    std::cout << "timing_file=";
    std::cout << (filenamePrefix.empty() ? "timing_not_enabled" : filenamePrefix) << "\n";
    std::cout << "num_replicas=" << getNumberOfTeams() << "\n";
    for (int i=0; i < getNumberOfTeams(); i++) {
      double rEndTime = 0.0;
      if (i == MASTER) {
        rEndTime = timer.endTime;
      } else {
        PMPI_Recv(&rEndTime, 1, MPI_DOUBLE, mapTeamToWorldRank(MASTER, i), 0, getLibComm(), MPI_STATUS_IGNORE);
      }

      std::cout << "replica " << i << "=" << rEndTime - timer.startTime << "s\n";
    }
    std::cout << "-------------------------------\n";
  }
  std::cout.flush();
  PMPI_Barrier(MPI_COMM_WORLD);

  if (!filenamePrefix.empty()) {
    // Write Generic Sync points to files
    char sep = ',';
    std::ostringstream filename;
    std::string outputFolder("tmpi-timings");
    filename << outputFolder << "/"
        << filenamePrefix << "-"
        << getWorldRank() << "-"
        << getTeamRank() << "-"
        << getTeam()
        << ".csv";
    std::ofstream f;
    f.open(filename.str().c_str());

    logInfo("Writing timings to " << filename);

    f << "endTime" << sep << timer.endTime - timer.startTime << "\n";

    f << "heartbeatTimes";
    for (const double& t : timer.heartbeatTimes.at(getTeam())) {
      f << sep << t - timer.startTime;
    }
    f << "\n";

    f << "heartbeatTimes";
    for (const double& t : timer.sleepPoints) {
      f << sep << t - timer.startTime;
    }
    f.close();
  }

  PMPI_Barrier(MPI_COMM_WORLD);
}

