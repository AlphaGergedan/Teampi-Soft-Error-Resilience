/*
 * Timing.h
 *
 *  Created on: 2 Mar 2018
 *      Author: ben
 */

#ifndef TIMING_H_
#define TIMING_H_

#include <mpi.h>
#include <vector>

void TMPI_Synchronise();

namespace Timing {

enum markType {
  Generic,
  Initialize,
  Finalize,
  Send,
  Recv,
  ISendStart,
  ISendFinish,
  IRecvStart,
  IRecvFinish,
  WaitStart,
  WaitFinish,
  BarrierStart,
  BarrierFinishtype
};

void markTimeline(markType type);

void initialise();

const std::vector<double>& getISendStartTimes();
const std::vector<double>& getISendEndTimes();
const std::vector<double>& getIRecvStartTimes();
const std::vector<double>& getIRecvEndTimes();
const std::vector<double>& getSyncPoints();


void outputTiming();

}

#endif /* TIMING_H_ */
