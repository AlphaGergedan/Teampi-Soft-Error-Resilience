/*
 * Timing.h
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#ifndef TIMING_H_
#define TIMING_H_

#include <mpi.h>
#include <vector>

void TMPI_Synchronise();

namespace Timing {

enum markType {
  Initialize,
  Finalize,
  Generic,
  // Below are unused for now
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

void compareProgressWithReplicas();

const std::vector<double>& getSyncPoints();

void outputTiming();

}

#endif /* TIMING_H_ */
