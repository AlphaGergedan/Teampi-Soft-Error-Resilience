/*
 * Timing.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: ben
 */

#include "Timing.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>

#include "RankOperations.h"
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
#ifdef TMPI_TIMING
    switch (type) {
      case Timing::markType::Initialize:
        timer.startTime = PMPI_Wtime();
        break;
      case Timing::markType::Finalize:
        timer.endTime = PMPI_Wtime() - timer.startTime;
        break;
//      case Timing::markType::Generic:
//        timer.syncPoints.push_back(PMPI_Wtime() - timer.startTime);
//        break;
      default:
        // Other unsupported options fall through
        break;
    }
#endif
}

void Timing::outputTiming() {
#ifdef TMPI_TIMING
  char sep = ',';
  std::ostringstream filename;
  filename << "timings-" << getWorldRank() << "-" << getTeamRank() << "-" << get_R_number(getWorldRank()) << ".csv";
  std::ofstream f;
  f.open(filename.str().c_str());

  logInfo("Writing timings to " << filename);

  f << "startTime" << sep << timer.startTime << "\n";
  f << "endTime" << sep << timer.endTime << "\n";

  f << "syncPoints";
  for (const auto& t : timer.syncPoints) {
    f << sep << t;
  }
  f << "\n";

  f.close();
#endif
}

