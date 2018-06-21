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

#include "RankOperations.h"
#include "TMPIConstants.h"
#include "Logging.h"

struct Timer {
  double startTime;
  double endTime;
  std::vector<double> iSendStart;
  std::vector<double> iSendEnd;
  std::vector<double> iRecvStart;
  std::vector<double> iRecvEnd;

  std::vector<double> syncPoints;

  std::set<MPI_Request*> sendLUT;
  std::set<MPI_Request*> recvLUT;
} timer;


void Timing::markTimeline(Timing::markType type) {
//#ifdef TMPI_TIMING
    switch (type) {
      case Timing::markType::Initialize:
        PMPI_Barrier(getReplicaCommunicator());
        timer.startTime = PMPI_Wtime();
        break;
      case Timing::markType::Finalize:
        PMPI_Barrier(getReplicaCommunicator());
        timer.endTime = PMPI_Wtime() - timer.startTime;
        break;
      case Timing::markType::Generic:
        timer.syncPoints.push_back(PMPI_Wtime() - timer.startTime);
        break;
      default:
        // Other unsupported options fall through
        break;
    }
//#endif
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

  f << "startTime" << sep << timer.startTime << "\n";
  f << "endTime" << sep << timer.endTime << "\n";

  f << "syncPoints";
  for (const double& t : timer.syncPoints) {
    f << sep << t;
  }
  f << "\n";

  f.close();

  PMPI_Barrier(MPI_COMM_WORLD);
}

