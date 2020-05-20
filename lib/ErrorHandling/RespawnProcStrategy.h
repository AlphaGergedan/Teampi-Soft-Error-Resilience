#ifndef RESPAWNPROCSTRATEGY_H_
#define RESPAWNPROCSTRATEGY_H_

#include "ErrorHandlingStrategy.h"
class RespawnProcStrategy : public ErrorHandler{
    public:
        RespawnProcStrategy();

    private:
    static void respawn_proc_errh_comm_team(MPI_Comm *pcomm, int *perr, ...);
    static void respawn_proc_errh_comm_world(MPI_Comm *pcomm, int *perr, ...);
    static void respawn_proc_recreate_comm_world(MPI_Comm comm);
};

#endif