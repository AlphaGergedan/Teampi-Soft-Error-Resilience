#ifndef RESPAWNPROCSTRATEGY_H_
#define RESPAWNPROCSTRATEGY_H_

    void respawn_proc_errh_comm_team(MPI_Comm *pcomm, int *perr, ...);
    void respawn_proc_errh_comm_world(MPI_Comm *pcomm, int *perr, ...);
    void respawn_proc_recreate_comm_world(MPI_Comm comm);

#endif