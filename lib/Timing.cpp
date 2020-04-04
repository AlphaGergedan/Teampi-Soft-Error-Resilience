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
#include <list>
#include <vector>

struct Timer {
  // PMPI_Wtime at start of execution
  double startTime;
  // PMPI_Wtime at the end of this ranks execution
  double endTime;

  // Mark when an application sleeps
  std::vector<double> sleepPoints;


  // TODO: add support for multiple tags (or do we need this?)
  // Delta times for each heartbeat (per replica)
  std::map< int, std::list<double> > heartbeatTimes;
  // Store the MPI_Requests for each heartbeat delta (per replica)
  std::map< int, std::list<MPI_Request> > heartbeatTimeRequests;

  // Hash for each heartbeat buffer (per replica)
  std::map<int, std::list<std::size_t> > heartbeatHashes;
  // Store the MPI_Requests for each heartbeat (per replica) 
  std::map<int, std::list<MPI_Request> > heartbeatHashRequests;
} timer;

void Timing::initialiseTiming() {
  synchroniseRanksInTeam();
  timer.startTime = PMPI_Wtime();
  for (int i=0; i < getNumberOfTeams(); i++) {
    timer.heartbeatTimes.insert({i, std::list<double>()});
    timer.heartbeatTimeRequests.insert({i, std::list<MPI_Request>()});

    timer.heartbeatHashes.insert({i, std::list<std::size_t>()});
    timer.heartbeatHashRequests.insert({i, std::list<MPI_Request>()});
  }
}

void Timing::finaliseTiming() {
  synchroniseRanksInTeam();
  timer.endTime = PMPI_Wtime();
}

void Timing::markTimeline(int tag) {
  if (tag > 0) {
    timer.heartbeatTimes.at(getTeam()).push_back(PMPI_Wtime());
  } else if (tag < 0) {
    if (timer.heartbeatTimes.at(getTeam()).size()) {
      timer.heartbeatTimes.at(getTeam()).back() = PMPI_Wtime() - timer.heartbeatTimes.at(getTeam()).back();
      //printf("World Rank: %d, team rank: %d, team: %d, submitted time %f\n", getWorldRank(), getTeamRank(), getTeam(),timer.heartbeatTimes.at(getTeam()).back());
      compareProgressWithReplicas();
    }
  } else {
    // TODO: if tag == 0 then single heartbeat mode not deltas
  }
}

void Timing::markTimeline(int tag, const void *sendbuf, int sendcount, MPI_Datatype sendtype) {
  markTimeline(tag);
  //compareBufferWithReplicas(sendbuf, sendcount, sendtype);
}


void Timing::progressOutstandingRequests(int targetTeam) {

    // Progress on outstanding receives and sends
    auto it = timer.heartbeatTimeRequests.at(targetTeam).begin();
    while (it != timer.heartbeatTimeRequests.at(targetTeam).end()) {
        int flag;
        PMPI_Test(&(*it), &flag, MPI_STATUS_IGNORE);
        if (flag) {
          if (!((*it) == MPI_REQUEST_NULL)){
            MPI_Request_free(&(*it));
          }
          it = timer.heartbeatTimeRequests.at(targetTeam).erase(it);
        }
        ++it;
    }
}

void Timing::pollForAndReceiveHeartbeat(int targetTeam) {
    int received = 0;
    PMPI_Iprobe(mapTeamToWorldRank(getTeamRank(),targetTeam), targetTeam, getLibComm(), &received, MPI_STATUS_IGNORE);
    if(received) {
      timer.heartbeatTimes.at(targetTeam).push_back(0.0);
      timer.heartbeatTimeRequests.at(targetTeam).push_back(MPI_Request());
      PMPI_Irecv(&timer.heartbeatTimes.at(targetTeam).back(), 1, MPI_DOUBLE,
               mapTeamToWorldRank(getTeamRank(), targetTeam), targetTeam, getLibComm(), &timer.heartbeatTimeRequests.at(targetTeam).back());
    }
}

void Timing::compareProgressWithReplicas() {
  for (int r=0; r < getNumberOfTeams(); r++) {
    if (r != getTeam()) {
      // Send out this replica's delta
      timer.heartbeatTimeRequests.at(r).push_back(MPI_Request());
      PMPI_Isend(&timer.heartbeatTimes.at(getTeam()).back(), 1, MPI_DOUBLE,
                mapTeamToWorldRank(getTeamRank(), r), getTeam(),
                getLibComm(), &timer.heartbeatTimeRequests.at(r).back());


      // Receive deltas from other replicas
      pollForAndReceiveHeartbeat(r); 
      progressOutstandingRequests(r);
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

      // // Test for completion of Irecv's
      // int numPending = 0;
      // for (int i=0; i < timer.heartbeatHashRequests.at(r).size(); i++) {
      //   int flag = 0;
      //   PMPI_Test(&timer.heartbeatHashRequests.at(r).at(i), &flag, MPI_STATUS_IGNORE);
      //   numPending += 1 - flag;
      // }
      // std::cout << "Num pending: " << numPending << "\n";
    }
  }
}

void Timing::sleepRankRaised() {
  timer.sleepPoints.push_back(PMPI_Wtime());
}


void Timing::outputTiming() {
  std::cout.flush();
  PMPI_Barrier(getLibComm());

  //finish outstanding communication requests
  bool finished_all = false;
  while(!finished_all) {
    finished_all = true;
    for(int r=0; r<getNumberOfTeams(); r++) {
      if(r!=getTeam()) {
        pollForAndReceiveHeartbeat(r);
        progressOutstandingRequests(r);
        finished_all &= timer.heartbeatTimeRequests.at(r).empty();
      }     
    }
  }

  std::string filenamePrefix = getEnvString("TMPI_FILE");
  std::string outputPathPrefix = getEnvString("TMPI_OUTPUT_PATH");

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
    //TODO hier böse
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
  PMPI_Barrier(getLibComm());

  if (!filenamePrefix.empty()) {
    // Write Generic Sync points to files
    char sep = ',';
    std::ostringstream filename;
    std::string outputFolder(outputPathPrefix.empty() ? "tmpi-timings" : outputPathPrefix);
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
      f << sep << t;
    }
    f << "\n";

    f << "sleepPoints";
    for (const double& t : timer.sleepPoints) {
      f << sep << t - timer.startTime;
    }
    f.close();
  }

  /*for (int r=0; r < getNumberOfTeams(); r++) {
    if (r != getTeam()) {
      for (const double& t : timer.heartbeatTimes.at()) {
        printf("World rank: %d, team rank: %d knows for replicand from team %d, time %f\n", getWorldRank(), getTeamRank(), r, t);
      }
    }
  }*/

  PMPI_Barrier(getLibComm());
}

