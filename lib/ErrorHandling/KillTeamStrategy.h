#ifndef KILLTEAMSTRATEGY_H_
#define KILLTEAMSTRATEGY_H_

#include "ErrorHandlingStrategy.h"
class KillTeamStrategy : public ErrorHandler{
    public:
        KillTeamStrategy();
    private:
    static void kill_team_errh_comm_world(MPI_Comm *pcomm, int *perr, ...);
    static void kill_team_errh_comm_team(MPI_Comm *pcomm, int *perr, ...);
};

#endif