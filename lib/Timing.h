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
void markTimeline(const void *sendbuf, int sendcount, MPI_Datatype sendtype);

void initialiseTiming();
void finaliseTiming();

void compareProgressWithReplicas();
void compareBufferWithReplicas(const void *sendbuf, int sendcount, MPI_Datatype sendtype);

const std::vector<double>& getSyncPoints();

void outputTiming();

}

#endif /* TIMING_H_ */
