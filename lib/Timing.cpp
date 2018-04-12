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

static double startTime = 0.0;

struct Timer {
  std::vector<double> iSendStart;
  std::vector<double> iSendEnd;
  std::vector<double> iRecvStart;
  std::vector<double> iRecvEnd;

  std::vector<double> syncPoints;

  std::set<MPI_Request*> sendLUT;
  std::set<MPI_Request*> recvLUT;
};

static struct Timer timer;

void TMPI_Synchronise() {
  timer.syncPoints.push_back(PMPI_Wtime() - startTime);
}

void Timing::initialise() {
  startTime = PMPI_Wtime();
}

const std::vector<double>& Timing::getISendStartTimes() {
  return timer.iSendStart;
}

const std::vector<double>& Timing::getISendEndTimes() {
  return timer.iSendEnd;
}

const std::vector<double>& Timing::getIRecvStartTimes() {
  return timer.iRecvStart;
}

const std::vector<double>& Timing::getIRecvEndTimes() {
  return timer.iRecvEnd;
}

const std::vector<double>& Timing::getSyncPoints() {
  return timer.syncPoints;
}

void Timing::startNonBlocking(Timing::NonBlockingType type, int tag, MPI_Request *request) {
  switch(type) {
    case Timing::NonBlockingType::iSend:
        timer.iSendStart.push_back(PMPI_Wtime()-startTime);
        timer.sendLUT.insert(request);
      break;

    case Timing::NonBlockingType::iRecv:
        timer.iRecvStart.push_back(PMPI_Wtime()-startTime);
        timer.recvLUT.insert(request);
      break;
    default:
      return;
  }
}

void Timing::endNonBlocking(MPI_Request *request, MPI_Status *status) {
    auto buf = timer.sendLUT.find(request);
    if (buf != timer.sendLUT.end()) {
      timer.iSendEnd.push_back(PMPI_Wtime()-startTime);
      timer.sendLUT.erase(buf);
    }
    buf = timer.recvLUT.find(request);
    if (buf != timer.recvLUT.end()) {
      timer.iRecvEnd.push_back(PMPI_Wtime()-startTime);
      timer.recvLUT.erase(buf);
    }
}

void Timing::outputTiming() {
  char sep = ',';
  std::ostringstream filename;
  filename << "timings-" << getWorldRank() << "-" << getTeamRank() << "-" << get_R_number(getWorldRank()) << ".csv";
  std::ofstream f;
  f.open(filename.str().c_str());

  logInfo("Writing timings to " << filename);

  f << "syncPoints";
  for (const auto& t : Timing::getSyncPoints()) {
    f << sep << t;
  }
  f << "\n";

  f << "iSendStart";
  for (const auto& t : Timing::getISendStartTimes()) {
    f << sep << t;
  }
  f << "\n";

  f << "iSendEnd";
  for (const auto& t : Timing::getISendEndTimes()) {
    f << sep << t;
  }
  f << "\n";

  f << "iRecvStart";
  for (const auto& t : Timing::getIRecvStartTimes()) {
    f << sep << t;
  }
  f << "\n";

  f << "iRecvEnd";
  for (const auto& t : Timing::getIRecvEndTimes()) {
    f << sep << t;
  }
  f << "\n";


  f.close();
}

