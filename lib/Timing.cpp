/*
 * Timing.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#include "Timing.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <stddef.h>

#include "RankOperations.h"
#include "TMPIConstants.h"
#include "Logging.h"

struct Timer {
  double startTime;
  double endTime;

  std::map< int, std::vector<double> > syncPoints;
  std::map< int, std::vector<MPI_Request> > requests;
} timer;



void Timing::initialiseTiming() {
  PMPI_Barrier(getReplicaCommunicator());
  timer.startTime = PMPI_Wtime();
  for (int i=0; i < getNumberOfReplicas(); i++) {
    timer.syncPoints.insert(std::make_pair(i,std::vector<double>()));
    timer.requests.insert(std::make_pair(i,std::vector<MPI_Request>()));
  }
}

void Timing::finaliseTiming() {
  PMPI_Barrier(getReplicaCommunicator());
  timer.endTime = PMPI_Wtime();
  // TODO Call output timing here?
}

void Timing::markTimeline() {
    timer.syncPoints.at(get_R_number()).push_back(PMPI_Wtime());
    compareProgressWithReplicas();
}

void Timing::compareProgressWithReplicas() {
  for (int r=0; r < getNumberOfReplicas(); r++) {
    if (r != get_R_number()) {
      // Send out this replicas times
      MPI_Request request;
      PMPI_Isend(&timer.syncPoints.at(get_R_number()).back(), 1, MPI_DOUBLE,
                map_team_to_world(getTeamRank(), r), get_R_number(),
                getTMPICommunicator(), &request);
      MPI_Request_free(&request);

      // Receive times from other replicas
      timer.syncPoints.at(r).push_back(0.0);
      timer.requests.at(r).push_back(MPI_Request());
      PMPI_Irecv(&timer.syncPoints.at(r).back(), 1, MPI_DOUBLE,
                 map_team_to_world(getTeamRank(), r), r, getTMPICommunicator(), &timer.requests.at(r).back());

      // Test for completion of Irecv's
      int numPending = 0;
      for (int i=0; i < timer.requests.at(r).size(); i++) {
        int flag = 0;
        PMPI_Test(&timer.requests.at(r).at(i), &flag, MPI_STATUS_IGNORE);
        numPending += 1 - flag;
      }
      logDebug("Number pending: " << numPending);
    }
  }


  //TODO: change value of 0 to optimal assert (still correct but not optimal)
//  int asrt =
//      MPI_MODE_NOSTORE    ||
//      MPI_MODE_NOPUT;
//  MPI_Win_fence(asrt, progressWin);
////  progress[get_R_number(getWorldRank())].lastSync = timer.syncPoints.back();
//  progress[get_R_number(getWorldRank())].lastSync = timer.syncPoints.back();
//  progress[get_R_number(getWorldRank())].syncID = timer.syncPoints.size();
//
//  for (int i=0; i < getNumberOfReplicas(); i++) {
//    if (i != get_R_number(getWorldRank())) {
//      MPI_Get(progress+i, 1, progressDatatype, map_team_to_world(getTeamRank(), i),
//              0, 1, progressDatatype, progressWin);
//    }
//  }
//
//  std::cout.flush();
//  std::cout << "Rank: " << getTeamRank() << " / " << getWorldRank() << "\n";
//  std::cout << "Iteration: " << timer.syncPoints.size() << "\n";
//  for (int i=0; i < getNumberOfReplicas(); i++) {
//    std::cout << "SyncID: " << progress[i].syncID << "\t TStamp: " << progress[i].lastSync - timer.startTime << "\n";
//  }
//  std::cout.flush();
  std::cout.flush();
  std::cout << "Rank " << getTeamRank() << "/" << getWorldRank() <<
      "\tIteration: " << timer.syncPoints.size() << "\n";
  std::cout.flush();
}

void Timing::outputTiming() {
  std::cout.flush();
  PMPI_Barrier(MPI_COMM_WORLD);

  // Output simple replica timings
  if ((getTeamRank() == MASTER) && (getWorldRank() != MASTER)) {
    PMPI_Send(&timer.endTime, 1, MPI_DOUBLE, MASTER, 0, getTMPICommunicator());
  }


  if (getWorldRank() == MASTER) {
    std::cout << std::endl;
    std::cout << "----------TMPI_TIMING----------\n";
    std::cout << "timing_file=";
#ifdef TMPI_TIMING
    std::cout << "tmpi_filename.csv";
#else
    std::cout << "timing_not_enabled";
#endif
    std::cout << "\n";
    std::cout << "num_replicas=" << getNumberOfReplicas() << "\n";
    for (int i=0; i < getNumberOfReplicas(); i++) {
      double rEndTime = 0.0;
      if (i == MASTER) {
        rEndTime = timer.endTime;
      } else {
        PMPI_Recv(&rEndTime, 1, MPI_DOUBLE, map_team_to_world(MASTER, i), 0, getTMPICommunicator(), MPI_STATUS_IGNORE);
      }

      std::cout << "replica_" << i << "=" << rEndTime << "\n";
    }
    std::cout << "-------------------------------\n";
  }
  std::cout.flush();
  PMPI_Barrier(MPI_COMM_WORLD);

  // Write Generic Sync points to files
  char sep = ',';
  std::ostringstream filename;
  std::string outputFolder("tmpi-timings");
  filename << outputFolder << "/"
      << "timings" << "-"
      << getWorldRank() << "-"
      << getTeamRank() << "-"
      << get_R_number(getWorldRank())
      << ".csv";
  std::ofstream f;
  f.open(filename.str().c_str());

  logInfo("Writing timings to " << filename);

  f << "endTime" << sep << timer.endTime - timer.startTime << "\n";

  f << "syncPoints";
  for (const double& t : timer.syncPoints.at(get_R_number())) {
    f << sep << t - timer.startTime;
  }
  f << "\n";

  f.close();

  PMPI_Barrier(MPI_COMM_WORLD);
}

