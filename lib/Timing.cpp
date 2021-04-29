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
#include <ostream>
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
  //std::cout << "Started init Timing" << std::endl;
  synchroniseRanksInTeam();
  timer.startTime = PMPI_Wtime();
  for (int i=0; i < getNumberOfTeams(); i++) { /* for each replica */
    timer.heartbeatTimes.insert({i, std::list<double>()});
    timer.heartbeatTimeRequests.insert({i, std::list<MPI_Request>()});

    timer.heartbeatHashes.insert({i, std::list<std::size_t>()});
    timer.heartbeatHashRequests.insert({i, std::list<MPI_Request>()});
  }
  //std::cout << "Inited Timing" << std::endl;
}

void Timing::finaliseTiming() {
  synchroniseRanksInTeam();
  timer.endTime = PMPI_Wtime();
}

void Timing::markTimeline(int tag) {
  /* positive tag means sending before task execution */
  if (tag > 0) {
    timer.heartbeatTimes.at(getTeam()).push_back(PMPI_Wtime());
  } else if (tag < 0) { /* negative tag means sending after task execution */
    if (timer.heartbeatTimes.at(getTeam()).size()) {
      timer.heartbeatTimes.at(getTeam()).back() = PMPI_Wtime() - timer.heartbeatTimes.at(getTeam()).back();
      /* TODO for debugging */
      printf("World Rank: %d, team rank: %d, team: %d, submitted time %f\n", getWorldRank(), getTeamRank(), getTeam(),timer.heartbeatTimes.at(getTeam()).back());
      compareProgressWithReplicas();
    }
  } else {
    // TODO: if tag == 0 then single heartbeat mode not deltas

    /* If tag == 0, this means that we only sent the hash
     * A possible single heartbeat mode ?
     * see below, compareBufferWithReplicas
     */
  }
}

void Timing::markTimeline(int tag, const void *sendbuf, int sendcount, MPI_Datatype sendtype) {
  markTimeline(tag);
  compareBufferWithReplicas(sendbuf, sendcount, sendtype); /* TODO */
}


void Timing::progressOutstandingRequests(int targetTeam) {

    // Progress on outstanding receives and sends
    auto it = timer.heartbeatTimeRequests.at(targetTeam).begin();
    while (it != timer.heartbeatTimeRequests.at(targetTeam).end()) { /* iterate all the pending send/receive requests */
        int flag;
        PMPI_Test(&(*it),             /* Request */
                  &flag,              /* Flag    */
                  MPI_STATUS_IGNORE); /* Status  */
        if (flag) {
          if (!((*it) == MPI_REQUEST_NULL)){
            MPI_Request_free(&(*it));
          }
          it = timer.heartbeatTimeRequests.at(targetTeam).erase(it); /* request finished, so we remove it */
        }
        ++it;
    }
}

void Timing::pollForAndReceiveHeartbeat(int targetTeam) {
    int received = 0;
    PMPI_Iprobe(mapTeamToWorldRank(getTeamRank(),targetTeam), /* Source */
                targetTeam,                                   /* Receive tag */
                getLibComm(),                                 /* Communicator */
                &received,                                    /* Flag */
                MPI_STATUS_IGNORE);                           /* Status */
    if(received) {
      timer.heartbeatTimes.at(targetTeam).push_back(0.0);
      timer.heartbeatTimeRequests.at(targetTeam).push_back(MPI_Request());
      PMPI_Irecv(&timer.heartbeatTimes.at(targetTeam).back(),         /* Receive buffer */
                 1,                                                   /* Receive count  */
                 MPI_DOUBLE,                                          /* Receive type   */
                 mapTeamToWorldRank(getTeamRank(), targetTeam),       /* Receive source */
                 targetTeam,                                          /* Receive tag    */
                 getLibComm(),                                        /* Communicator   */
                 &timer.heartbeatTimeRequests.at(targetTeam).back()); /* Request        */
    }
}

void Timing::compareProgressWithReplicas() {
  for (int r=0; r < getNumberOfTeams(); r++) { /* for each other replica */
    if (r != getTeam()) {
      // Send out this replica's delta
      timer.heartbeatTimeRequests.at(r).push_back(MPI_Request());
      PMPI_Isend(&timer.heartbeatTimes.at(getTeam()).back(), /* Send buffer  */
                 1,                                          /* Send count   */
                 MPI_DOUBLE,                                 /* Send type    */
                 mapTeamToWorldRank(getTeamRank(), r),       /* Destination  */
                 getTeam(),                                  /* Send tag     */
                 getLibComm(),                               /* Communicator */
                 &timer.heartbeatTimeRequests.at(r).back()); /* Request      */
      /* Why not free here ? */


      // Receive deltas from other replicas
      pollForAndReceiveHeartbeat(r);
      progressOutstandingRequests(r);
    }
  }
}


/* TODO */
void Timing::progressOutstandingHashRequests(int targetTeam) {
    // Progress on outstanding receives and sends
    auto it = timer.heartbeatHashRequests.at(targetTeam).begin();
    while (it != timer.heartbeatHashRequests.at(targetTeam).end()) { /* iterate all the pending send/receive requests */
        int flag;
        PMPI_Test(&(*it),             /* Request */
                  &flag,              /* Flag    */
                  MPI_STATUS_IGNORE); /* Status  */
        if (flag) {
          if (!((*it) == MPI_REQUEST_NULL)){
            MPI_Request_free(&(*it));
          }
          // compare the hashes
          // ASSUMPTION : If a request is finished, then we should have the hash
          //              as well. TODO check this assumption
          if (timer.heartbeatHashes.at(getTeam()).front() == timer.heartbeatHashes.at(targetTeam).front()) {
            std::cout << "\n" << "-----------------------------> Hash buffers equal : "
                      << timer.heartbeatHashes.at(getTeam()).front()
                      << std::endl;
          } else {
            std::cout << "\n" << "-----------------------------> Hash buffers NOT EQUAL : "
                      << timer.heartbeatHashes.at(getTeam()).front() << " (replica: "
                      << getTeam() << ")" << " != "
                      << timer.heartbeatHashes.at(targetTeam).front() << " (replica: "
                      << targetTeam << ")" << std::endl;

            std::cout << "Aborting.." << std::endl;
            PMPI_Abort(getLibComm(), MPI_ERR_BUFFER);
          }
          /* pop? or do we need them later?? */
          timer.heartbeatHashes.at(getTeam()).pop_front();
          timer.heartbeatHashes.at(targetTeam).pop_front();

          it = timer.heartbeatHashRequests.at(targetTeam).erase(it); /* request finished, so we remove it */
        }
        ++it;
    }
}

/* TODO */
void Timing::pollForAndReceiveHash(int targetTeam) {
    int received = 0;
    PMPI_Iprobe(mapTeamToWorldRank(getTeamRank(),targetTeam), /* Source */
                targetTeam+getNumberOfTeams(),                /* Receive tag */ // with the offset
                getLibComm(),                                 /* Communicator */
                &received,                                    /* Flag */
                MPI_STATUS_IGNORE);                           /* Status */
    if(received) {
      timer.heartbeatHashes.at(targetTeam).push_back(0.0);
      timer.heartbeatHashRequests.at(targetTeam).push_back(MPI_Request());
      PMPI_Irecv(&timer.heartbeatHashes.at(targetTeam).back(),        /* Receive buffer */
                 1,                                                   /* Receive count  */
                 MPI_DOUBLE,                                          /* Receive type   */
                 mapTeamToWorldRank(getTeamRank(), targetTeam),       /* Receive source */
                 targetTeam+getNumberOfTeams(),                       /* Receive tag    */ // with the offset TODO
                 getLibComm(),                                        /* Communicator   */
                 &timer.heartbeatHashRequests.at(targetTeam).back()); /* Request        */
      std::cout << "\nSTATUS: self (" << getTeam() << ") : ";
      for (const size_t &tmp : timer.heartbeatHashes.at(getTeam())) {
        std::cout << tmp << ", ";
      }
      std::cout << " END" << std::endl;
      std::cout << "STATUS: replica (" << targetTeam << ") : ";
      for (const size_t &tmp : timer.heartbeatHashes.at(targetTeam)) {
        std::cout << tmp << ", ";
      }
      std::cout << " END" << std::endl;
    }
}



void Timing::compareBufferWithReplicas(const void *sendbuf, int sendcount, MPI_Datatype sendtype) {
  if (getShouldCorruptData()) {
    //TODO can remove const here via cast (assuming data was originally non-const) and corrupt properly, no need for now
    sendcount++; // This isn't really that safe either...likely causes memory corruption occasionally
    setShouldCorruptData(false);
  }

  /* TODO it is already hashed */

  int typeSize;
  MPI_Type_size(sendtype, &typeSize);

  std::string bits((const char*)sendbuf, sendcount*typeSize);

  std::hash<std::string> hash_fn; /* TODO: we already hashed the results, don't have to hash here */
  std::size_t hash = hash_fn(bits);

  // for debugging TODO
  std::cout << "\n\n-------------------------->"
            << "HASH FROM REPLICA: " << getTeam() << " = " << *((size_t*) sendbuf)
            << "\n-------------------------->"
            << " is hashed in tmpi --> " << hash
            << std::endl;

  /* Store this replica's hash */
  timer.heartbeatHashes.at(getTeam()).push_back((std::size_t)hash);

  for (int r=0; r < getNumberOfTeams(); r++) {
    if (r != getTeam()) {
      // Send out this replica's hashes
      MPI_Request request;
      PMPI_Isend(&timer.heartbeatHashes.at(getTeam()).back(), /* Send buffer  */
                 1,                                           /* Send count   */
                 TMPI_SIZE_T,                                 /* Send type    */
                 mapTeamToWorldRank(getTeamRank(), r),        /* Destination  */
                 getTeam()+getNumberOfTeams(),                /* Send tag     */ // TODO is this differentiation good? With offset numTeams
                 getLibComm(),                                /* Communicator */
                 &request);                                   /* Request      */
      MPI_Request_free(&request); // TODO why free here ? We did not free in compare buffer func.

      // Receive hashes from other replicas
      pollForAndReceiveHash(r);
      progressOutstandingHashRequests(r);
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
        /* Receive remaining heartbeats */
        pollForAndReceiveHeartbeat(r);
        progressOutstandingRequests(r);

        /* Receive remaining hashes */
        pollForAndReceiveHash(r);
        progressOutstandingHashRequests(r);

        /* check if both are finished */
        finished_all &=
            timer.heartbeatTimeRequests.at(r).empty() && timer.heartbeatHashRequests.at(r).empty();
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

    f << "hashes";
    for (const size_t& h : timer.heartbeatHashes.at(getTeam())) {
        f << sep << h;
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

