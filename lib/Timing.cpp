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

#include "Logging.h"
#include "Rank.h"

struct Timer {
  double startTime;
  double endTime;

  std::map< int, std::vector<double> > syncPoints;
  std::map< int, std::vector<MPI_Request> > requests;
} timer;



void Timing::initialiseTiming() {
  synchroniseRanksInTeam();
  timer.startTime = PMPI_Wtime();
  for (int i=0; i < getNumberOfTeams(); i++) {
    timer.syncPoints.insert(std::make_pair(i,std::vector<double>()));
    timer.requests.insert(std::make_pair(i,std::vector<MPI_Request>()));
  }
}

void Timing::finaliseTiming() {
  synchroniseRanksInTeam();
  timer.endTime = PMPI_Wtime();
}

void Timing::markTimeline() {
    timer.syncPoints.at(getTeam()).push_back(PMPI_Wtime());
    compareProgressWithReplicas();
}

void Timing::compareProgressWithReplicas() {
  for (int r=0; r < getNumberOfTeams(); r++) {
    if (r != getTeam()) {
      // Send out this replica's times
      MPI_Request request;
      PMPI_Isend(&timer.syncPoints.at(getTeam()).back(), 1, MPI_DOUBLE,
                mapTeamToWorldRank(getTeamRank(), r), getTeam(),
                getLibComm(), &request);
      MPI_Request_free(&request);

      // Receive times from other replicas
      timer.syncPoints.at(r).push_back(0.0);
      timer.requests.at(r).push_back(MPI_Request());
      PMPI_Irecv(&timer.syncPoints.at(r).back(), 1, MPI_DOUBLE,
                 mapTeamToWorldRank(getTeamRank(), r), r, getLibComm(), &timer.requests.at(r).back());

      // Test for completion of Irecv's
      int numPending = 0;
      for (int i=0; i < timer.requests.at(r).size(); i++) {
        int flag = 0;
        PMPI_Test(&timer.requests.at(r).at(i), &flag, MPI_STATUS_IGNORE);
        numPending += 1 - flag;
      }
    }
  }
}

void Timing::outputTiming() {
  std::cout.flush();
  PMPI_Barrier(MPI_COMM_WORLD);

  // Output simple replica timings
  if ((getTeamRank() == MASTER) && (getWorldRank() != MASTER)) {
    PMPI_Send(&timer.endTime, 1, MPI_DOUBLE, MASTER, 0, getLibComm());
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
    std::cout << "num_replicas=" << getNumberOfTeams() << "\n";
    for (int i=0; i < getNumberOfTeams(); i++) {
      double rEndTime = 0.0;
      if (i == MASTER) {
        rEndTime = timer.endTime;
      } else {
        PMPI_Recv(&rEndTime, 1, MPI_DOUBLE, mapTeamToWorldRank(MASTER, i), 0, getLibComm(), MPI_STATUS_IGNORE);
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
      << getTeam()
      << ".csv";
  std::ofstream f;
  f.open(filename.str().c_str());

  logInfo("Writing timings to " << filename);

  f << "endTime" << sep << timer.endTime - timer.startTime << "\n";

  f << "syncPoints";
  for (const double& t : timer.syncPoints.at(getTeam())) {
    f << sep << t - timer.startTime;
  }
  f << "\n";

  f.close();

  PMPI_Barrier(MPI_COMM_WORLD);
}

