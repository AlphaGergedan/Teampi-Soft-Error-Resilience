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

/*TeaMPI expects the application to reload a checkpoint from fs and be ready to continue
*execution from saved state
*/
void TMPI_SetLoadCheckpointCallback(std::function<void(void)>*);
/*TeaMPI expects the application to create a checkpoint that can be loaded with above function*/
void TMPI_SetCreateCheckpointCallback(std::function<void(void)>*);
MPI_Comm TMPI_GetInterTeamComm();
int TMPI_GetTeamNumber();
int TMPI_GetWorldRank();
int TMPI_GetInterTeamCommSize();
int TMPI_IsLeadingRank();

#endif