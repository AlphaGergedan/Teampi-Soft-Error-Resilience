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
  std::vector<double> syncPoints;
} timer;

struct Progress {
  int syncID;
  double lastSync;
} *progress;

static MPI_Win progressWin;
static MPI_Datatype progressDatatype;


void Timing::markTimeline(Timing::markType type) {
    switch (type) {
      case Timing::markType::Initialize:
        PMPI_Barrier(getReplicaCommunicator());
        timer.startTime = PMPI_Wtime();
        initialiseTiming();
        break;
      case Timing::markType::Finalize:
        PMPI_Barrier(getReplicaCommunicator());
        timer.endTime = PMPI_Wtime();
        break;
      case Timing::markType::Generic:
        timer.syncPoints.push_back(PMPI_Wtime());
        compareProgressWithReplicas();
        break;
      default:
        // Other unsupported options fall through
        break;
    }
}

void Timing::initialiseTiming() {
  progress = (Progress*)malloc(sizeof(Progress)*getNumberOfReplicas());

  // Initialise Progress datatype
  const int nitems = 2;
  int blocklengths[2] = {1, 1};
  MPI_Datatype types[2] = {MPI_INT, MPI_DOUBLE};
  MPI_Aint offsets[2];
  offsets[0] = offsetof(Progress, syncID);
  offsets[1] = offsetof(Progress, lastSync);
  MPI_Type_create_struct(nitems, blocklengths, offsets, types, &progressDatatype);
  MPI_Type_commit(&progressDatatype);

  // Allocate RMA window
  int typeSize;
  MPI_Type_size(progressDatatype, &typeSize);
  MPI_Win_create(progress+(get_R_number(getWorldRank())), 1, typeSize, MPI_INFO_NULL, getTMPICommunicator(), &progressWin);
}

void Timing::compareProgressWithReplicas() {
  //TODO: change value of 0 to optimal assert (still correct but not optimal)
  int asrt =
      MPI_MODE_NOSTORE    ||
      MPI_MODE_NOPUT;
  MPI_Win_fence(asrt, progressWin);
//  progress[get_R_number(getWorldRank())].lastSync = timer.syncPoints.back();
  progress[get_R_number(getWorldRank())].lastSync = timer.syncPoints.back();
  progress[get_R_number(getWorldRank())].syncID = timer.syncPoints.size();

  for (int i=0; i < getNumberOfReplicas(); i++) {
    if (i != get_R_number(getWorldRank())) {
      MPI_Get(progress+i, 1, progressDatatype, map_team_to_world(getTeamRank(), i),
              0, 1, progressDatatype, progressWin);
    }
  }

  std::cout.flush();
  std::cout << "Rank: " << getTeamRank() << " / " << getWorldRank() << "\n";
  std::cout << "Iteration: " << timer.syncPoints.size() << "\n";
  for (int i=0; i < getNumberOfReplicas(); i++) {
    std::cout << "SyncID: " << progress[i].syncID << "\t TStamp: " << progress[i].lastSync - timer.startTime << "\n";
  }
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
  for (const double& t : timer.syncPoints) {
    f << sep << t - timer.startTime;
  }
  f << "\n";

  f.close();

  PMPI_Barrier(MPI_COMM_WORLD);
}

