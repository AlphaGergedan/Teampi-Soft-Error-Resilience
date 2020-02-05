/*
 * CommStats.h
 *
 *  Created on: 4 Feb 2020
 *      Author: Philipp Samfass
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <mpi.h>


namespace CommunicationStatistics {

 size_t computeCommunicationVolume(MPI_Datatype, int count);

 void trackSend(MPI_Datatype datatype, int count);
 void trackReceive(MPI_Datatype datatype, int count); 

 void outputCommunicationStatistics();

}

#endif /* STATISTICS_H_ */

