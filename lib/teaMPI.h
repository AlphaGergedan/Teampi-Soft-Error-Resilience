/**
 * teaMPI.h
 *
 * Created on 19 Aug 2019
 * Author: Philipp Samfass
 */
#ifndef TEAMPI_H_
#define TEAMPI_H_

#include <mpi.h>
#include <functional>
#include "ErrorHandling/ErrorHandlingStrategies.h"


/*TeaMPI expects the application to reload a checkpoint from fs and be ready to continue
*execution from saved state
*Parameter: true if process is a new spawn
*/
void TMPI_SetLoadCheckpointCallback(std::function<void(bool)>*);
/*TeaMPI expects the application to create a checkpoint that can be loaded with above function*/
void TMPI_SetCreateCheckpointCallback(std::function<void(void)>*);
/**/
void TMPI_SetErrorHandlingStrategy(TMPI_ErrorHandlingStrategy strategy);
MPI_Comm TMPI_GetInterTeamComm();
MPI_Comm TMPI_GetWorldComm();
int TMPI_GetTeamNumber();
int TMPI_GetWorldRank();
int TMPI_GetInterTeamCommSize();
int TMPI_IsLeadingRank();

#endif