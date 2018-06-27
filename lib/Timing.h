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

namespace Timing {

void markTimeline();

void initialiseTiming();
void finaliseTiming();

void compareProgressWithReplicas();

const std::vector<double>& getSyncPoints();

void outputTiming();

}

#endif /* TIMING_H_ */
