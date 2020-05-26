#include "WarmSpareStrategy.h"

#include <mpi-ext.h>
#include <iostream>
#include <set>
#include <cassert>
#include <vector>
#include <unordered_map>

#include "../Rank.h"
#include "../Timing.h"

void warm_spare_errh(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm;
    int eclass, rank_team, team;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        std::cout << "Aborting: Unexpected Error" << eclass << std::endl;
        MPI_Abort(comm, err);
    }

    rank_team = getTeamRank();
    team = getTeam();
    std::cout << "Errorhandler warm_spare_errh invoked on " << rank_team << " of team: " << team << std::endl;
    PMPIX_Comm_revoke(getWorldComm());
    PMPIX_Comm_revoke(getLibComm());
    PMPIX_Comm_revoke(getTeamComm(MPI_COMM_WORLD));

    warm_spare_rejoin_function(false);
}

void warm_spare_wait_function()
{
    int send = 1000;
    int recv = 0;
    int err = 0;
    std::cout << "Waiting: " << getWorldRank() << std::endl;

    while (recv < 1000 || err != MPI_SUCCESS)
    {
        send = 1000;
        err = PMPI_Allreduce(&send, &recv, 1, MPI_INT, MPI_MIN, getLibComm());
        int revoked;
        int size;
        PMPIX_Comm_is_revoked(getLibComm(), &revoked);
        if (revoked)
        {
            std::cout << revoked << std::endl;
            continue;
        }
        send = recv;
        PMPI_Allreduce(&send, &recv, 1, MPI_INT, MPI_MIN, getTeamComm(MPI_COMM_WORLD));
        PMPI_Comm_size(getWorldComm(), &size);
        std::cout << "(Spare) Allred recv: " << recv << " Size: " << size << " error: " << err << " Rank: " << getWorldRank() << std::endl;
    }

    Timing::outputTiming();
    PMPI_Finalize();
    std::cout << "Spare Finalized and is now exiting" << std::endl;
    std::exit(0);
}

void warm_spare_rejoin_function(bool isSpareRank)
{
    MPI_Comm comm_world_shrinked, comm_world_cleaned;
    MPI_Comm comm_team_new, comm_lib_new;
    MPI_Group group_world, group_world_shrinked, group_failed;
    int size_world, size_world_shrinked, num_failed, current_num_spares;
    int failed_normal = 0, failed_spares = 0;
    int size_without_spares;
    int rank_world, rank_world_shrinked, rank_new;
    int flag = 1;
    int error;
    bool used_spare = false;
    int reload_team;
    std::unordered_map<int, int> failed_teams;

redo:

    PMPIX_Comm_shrink(getWorldComm(), &comm_world_shrinked);

    PMPI_Comm_size(getWorldComm(), &size_world);
    PMPI_Comm_size(comm_world_shrinked, &size_world_shrinked);
    num_failed = size_world - size_world_shrinked;

    PMPI_Comm_set_errhandler(getWorldComm(), MPI_ERRORS_RETURN);
    PMPI_Comm_set_errhandler(getTeamComm(MPI_COMM_WORLD), MPI_ERRORS_RETURN);


    PMPIX_Comm_agree(comm_world_shrinked, &flag);
    if (!flag)
    {
        PMPI_Comm_free(&comm_world_shrinked);
        goto redo;
    }

    PMPI_Comm_rank(getWorldComm(), &rank_world);
    PMPI_Comm_rank(comm_world_shrinked, &rank_world_shrinked);

    PMPI_Comm_group(getWorldComm(), &group_world);
    PMPI_Comm_group(comm_world_shrinked, &group_world_shrinked);
    PMPI_Group_difference(group_world, group_world_shrinked, &group_failed);

    std::vector<int> failed_ranks;
    failed_ranks.resize(num_failed);
    for (int i = 0; i <= getNumberOfTeams(); i++)
    {
        failed_teams[i] = 0;
    }

    for (int i = 0; i < num_failed; i++)
    {   
        int diff_rank;
        PMPI_Group_translate_ranks(group_failed, 1, &i, group_world, &diff_rank);
        failed_ranks.at(i) = diff_rank;
        int current_team = mapRankToTeamNumber(failed_ranks.at(i));
        (current_team == getNumberOfTeams()) ? ++failed_spares : ++failed_normal;
        failed_teams[current_team]++;
        std::cout << rank_world << " failed team: " << current_team << std::endl;
    }

    for (int i = 0; i <= getNumberOfTeams(); i++)
    {
        if (failed_teams[i] == 0)
        {
            std::cout << "Reload Team: " << i << std::endl;
            reload_team = i;
            break;
        }
        if (i == getNumberOfTeams())
        {
            std::cout << "Abort 1" << std::endl;
            MPI_Abort(comm_world_shrinked, MPI_ERR_INTERN);
        }
    }

    MPI_Group_free(&group_failed);
    MPI_Group_free(&group_world);
    MPI_Group_free(&group_world_shrinked);

    current_num_spares = getNumberOfSpares() - failed_spares;
    if (failed_normal > current_num_spares)
    {
        printf("%d failed (%d, %d), but only %d spares exist, aborting...\n",num_failed, failed_normal, failed_spares, current_num_spares);
        MPI_Abort(comm_world_shrinked, MPI_ERR_INTERN);
    }

    int key = rank_world;
    if (isSpare())
    {
        int teamRank;
        PMPI_Comm_rank(getTeamComm(MPI_COMM_WORLD), &teamRank);
        if (teamRank < num_failed)
        {
            key = failed_ranks.at(teamRank);
            std::cout << "(Spare) rank: " << rank_world << " going to become: " << key << std::endl; 
            setSpare(false);
        }
        else
        {
            key = size_world;
        }
    }

    error = PMPI_Comm_split(comm_world_shrinked, 1, key, &comm_world_cleaned);
    flag = (MPI_SUCCESS == error);
    PMPIX_Comm_agree(comm_world_shrinked, &flag);
    if (!flag)
    {
        PMPI_Comm_free(&comm_world_cleaned);
        goto redo;
    }

    current_num_spares -= failed_normal;
    size_without_spares = size_world_shrinked - current_num_spares;
    setNumberOfSpares(current_num_spares);

    printf("%d/%d/%d/%d/%d\n", current_num_spares, size_without_spares, failed_normal, getTeamSize(), getNumberOfTeams());
   

    PMPI_Comm_rank(comm_world_cleaned, &rank_new);
    int color = (rank_new >= size_without_spares) ? getNumberOfTeams() : rank_new / getTeamSize();

    PMPI_Comm_split(comm_world_cleaned, color, rank_new, &comm_team_new);
    PMPI_Comm_dup(comm_world_cleaned, &comm_lib_new);

    MPI_Errhandler errh;
    PMPI_Comm_create_errhandler(warm_spare_errh, &errh);
    PMPI_Comm_set_errhandler(comm_lib_new, errh);
    PMPI_Comm_set_errhandler(comm_world_cleaned, errh);
    PMPI_Comm_set_errhandler(comm_team_new, errh);

    setTeam(color);
    setTeamComm(comm_team_new);
    setWorldComm(comm_world_cleaned);
    setLibComm(comm_lib_new);

    //Falsch bei spares
    if(failed_teams[getTeam()] == 0)assert(getTeamSize() == size_without_spares / getNumberOfTeams());

    if (getTeam() == reload_team)
    {
        (*(getCreateCheckpointCallback()))();
    }

    if (failed_teams[getTeam()] > 0)
    {
        Timing::initialiseTiming();
        (*(getLoadCheckpointCallback()))(false);
    }
}