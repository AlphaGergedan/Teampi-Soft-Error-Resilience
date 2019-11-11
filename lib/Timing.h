/*
 * Timing.h
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#ifndef TIMING_H_
#define TIMING_H_

#include <mpi.h>


namespace Timing {

// Mark time only for this heartbeat
void markTimeline(int tag);
// Also mark the hash for the heartbeat buffer
void markTimeline(int tag, const void *sendbuf, int sendcount, MPI_Datatype sendtype);

void initialiseTiming();
void finaliseTiming();

// Compare the time of heartbeat(s) with other replica(s)
void compareProgressWithReplicas();
// Also compare a hash of a heartbeat buffer
void compareBufferWithReplicas(const void *sendbuf, int sendcount, MPI_Datatype sendtype);

void progressOutstandingRequests(int targetTeam);

void sleepRankRaised();

void outputTiming();
}

#endif /* TIMING_H_ */
