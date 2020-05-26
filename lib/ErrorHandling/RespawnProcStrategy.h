#ifndef RESPAWNPROCSTRATEGY_H_
#define RESPAWNPROCSTRATEGY_H_

#include <mpi.h>

    void respawn_proc_errh(MPI_Comm *pcomm, int *perr, ...);
    void respawn_proc_recreate_world(bool newSpawn);

#endif