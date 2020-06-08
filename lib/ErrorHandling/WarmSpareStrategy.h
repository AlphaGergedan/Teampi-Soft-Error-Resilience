#ifndef WARMSPARESTRATEGY_H_
#define WARMSPARESTRATEGY_H_

#include <mpi.h>
#include <unordered_map>
#include <vector>

    void warm_spare_errh(MPI_Comm *pcomm, int *perr, ...);
    void warm_spare_recreate_world(bool  isSpare);
    void warm_spare_wait_function();
    void get_failed_teams_and_ranks(std::vector<int> *failed_ranks, std::unordered_map<int, int> *failed_teams, MPI_Comm shrinked_comm);
    void get_failed_proc_type(std::unordered_map<int, int> *failed_teams, int *failed_normal, int *failed_spares);
    int get_reload_team(std::unordered_map<int, int> *failed_teams);
#endif