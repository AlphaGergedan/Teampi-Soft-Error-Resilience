/*
 * Timing.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: ben
 */

#include "Timing.h"
#include "TMPIConstants.h"

#include <vector>
#include <set>

static double startTime = 0.0;

struct Timer {
  std::vector<double> iSendStart;
  std::vector<double> iSendEnd;
  std::vector<double> iRecvStart;
  std::vector<double> iRecvEnd;

  std::set<MPI_Request*> sendLUT;
  std::set<MPI_Request*> recvLUT;
};

static struct Timer timer;

void Timing::initialise() {
  startTime = MPI_Wtime();
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

void Timing::startNonBlocking(Timing::NonBlockingType type, int tag, MPI_Request *request) {
  switch(type) {
    case Timing::NonBlockingType::iSend:
      if (tag == ITAG) {
        timer.iSendStart.push_back(MPI_Wtime()-startTime);
        timer.sendLUT.insert(request);
      }
      break;

    case Timing::NonBlockingType::iRecv:
      if ((tag == ITAG) || (tag == MPI_ANY_TAG)) {
        timer.iRecvStart.push_back(MPI_Wtime()-startTime);
        timer.recvLUT.insert(request);
      }
      break;
    default:
      return;
  }
}

void Timing::endNonBlocking(MPI_Request *request, MPI_Status *status) {
  if (status->MPI_TAG == ITAG) {
    auto buf = timer.sendLUT.find(request);
    if (buf != timer.sendLUT.end()) {
      timer.iSendEnd.push_back(MPI_Wtime()-startTime);
      timer.sendLUT.erase(buf);
    }
    buf = timer.recvLUT.find(request);
    if (buf != timer.recvLUT.end()) {
      timer.iRecvEnd.push_back(MPI_Wtime()-startTime);
      timer.recvLUT.erase(buf);
    }
  }
}

