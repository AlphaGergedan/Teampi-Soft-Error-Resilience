#ifndef KILLTEAMSTRATEGY_H_
#define KILLTEAMSTRATEGY_H_

#include <mpi.h>

    void kill_team_errh_comm_world(MPI_Comm *pcomm, int *perr, ...);
    void kill_team_recreate_world(bool isSpareRank);
    
#endif