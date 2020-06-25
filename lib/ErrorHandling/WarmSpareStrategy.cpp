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
    std::cout << "Errorhandler warm_spare_errh invoked on " << rank_team << " of team: " << team << " with error: " << eclass << std::endl;
    PMPIX_Comm_revoke(getWorldComm());
    PMPIX_Comm_revoke(getLibComm());
    PMPIX_Comm_revoke(getTeamComm(MPI_COMM_WORLD));

    warm_spare_recreate_world(false);
    std::cout << "Errh returning" << std::endl;
    return;
}

void warm_spare_wait_function()
{
    int send = 1000;
    int recv = 0;
    int err = 0;
    std::cout << "Waiting: " << getWorldRank() << std::endl;

    while (recv < 1000 || err != MPI_SUCCESS)
    {
        int size;
        PMPI_Comm_set_errhandler(getTeamComm(MPI_COMM_WORLD), MPI_ERRORS_RETURN);
        PMPI_Comm_set_errhandler(getLibComm(), MPI_ERRORS_RETURN);

        err = PMPI_Allreduce(&send, &recv, 1, MPI_INT, MPI_MIN, getLibComm());
        int flag = (err == MPI_SUCCESS);
        PMPIX_Comm_agree(getTeamComm(MPI_COMM_WORLD), &flag);
        if (!flag)
        {
            (*getRecreateWorldFunction())(false);
        }
        PMPI_Comm_set_errhandler(getTeamComm(MPI_COMM_WORLD), *getTeamErrhandler());
        PMPI_Comm_set_errhandler(getLibComm(), *getTeamErrhandler());
        /*         send = 1000;
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
        PMPI_Allreduce(&send, &recv, 1, MPI_INT, MPI_MIN, getTeamComm(MPI_COMM_WORLD));*/
        PMPI_Comm_size(getWorldComm(), &size);
        //std::cout << "(Spare) Allred recv: " << recv << " Size: " << size << " error: " << err << " Rank: " << getWorldRank() << std::endl;
    }

    Timing::outputTiming();
    #ifdef DirtyCleanUp
    std::cout << "Spare: " << getTeamRank() << " finalizing" << std::endl;
    MPI_Barrier(getLibComm());
    std::cout << "Spare: " << getTeamRank() << " finalized and is now exiting" << std::endl;
    std::exit(0);
    #else
    std::cout << "Spare: " << getTeamRank() << " finalizing" << std::endl;
    PMPI_Finalize();
    std::cout << "Spare Finalized and is now exiting" << std::endl;
    std::exit(0);
    #endif
    
}

void warm_spare_recreate_world(bool isSpareRank)
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
    int reload_team;
    bool spare = isSpare();
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

    for (int i = 0; i <= getNumberOfTeams(); i++)
    {
        failed_teams[i] = 0;
    }

    get_failed_teams_and_ranks(&failed_ranks, &failed_teams, comm_world_shrinked);
    get_failed_proc_type(&failed_teams, &failed_normal, &failed_spares);
    reload_team = get_reload_team(&failed_teams);

    MPI_Group_free(&group_failed);
    MPI_Group_free(&group_world);
    MPI_Group_free(&group_world_shrinked);

    current_num_spares = getNumberOfSpares() - failed_spares;

    //printf("Status 1: %d/%d/%d/%d/%d/%d\n", current_num_spares, size_without_spares, failed_normal, failed_spares, getTeamSize(), getNumberOfTeams());
    if (failed_normal > current_num_spares)
    {
        printf("%d failed (%d, %d), but only %d spares exist, aborting...\n", num_failed, failed_normal, failed_spares, current_num_spares);
        MPI_Abort(comm_world_shrinked, MPI_ERR_INTERN);
        assert(false);
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
            if (key < getNumberOfTeams() * getTeamSize())
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

    printf("Status 2: %d/%d/%d/%d/%d/%d\n", current_num_spares, size_without_spares, failed_normal, failed_spares, getTeamSize(), getNumberOfTeams());

    PMPI_Comm_rank(comm_world_cleaned, &rank_new);
    int color = (rank_new >= size_without_spares) ? getNumberOfTeams() : rank_new / getTeamSize();
    int team_rank;

    PMPI_Comm_split(comm_world_cleaned, color, rank_new, &comm_team_new);
    PMPI_Comm_dup(comm_world_cleaned, &comm_lib_new);
    PMPI_Comm_rank(comm_team_new, &team_rank);

    MPI_Errhandler errh;
    PMPI_Comm_create_errhandler(warm_spare_errh, &errh);
    PMPI_Comm_set_errhandler(comm_lib_new, errh);
    PMPI_Comm_set_errhandler(comm_world_cleaned, errh);
    PMPI_Comm_set_errhandler(comm_team_new, errh);

    setTeam(color);
    setTeamRank(team_rank);
    setTeamComm(comm_team_new);
    setWorldComm(comm_world_cleaned);
    setLibComm(comm_lib_new);
    refreshWorldSize();
    refreshWorldRank();

    printf("New Rank: %d/%d/%d\n", team_rank, getWorldRank(), rank_world);

    //Falsch bei spares
    if (failed_teams[getTeam()] == 0)
        assert(getTeamSize() == size_without_spares / getNumberOfTeams());

    if (failed_normal == 0)
    {
        std::cout << "only spares failed, returning..." << std::endl;
        return;
    }

    std::vector<int> failed_team_vector;
    for (const auto &i : failed_teams)
    {
        if (i.second > 0)
            failed_team_vector.push_back(i.first);
    }

    if (getTeam() == reload_team)
    {
        if (spare)
        {
            std::cout << "spare attempting to load checkpoint" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_INTERN);
        }
        (*(getCreateCheckpointCallback()))(failed_team_vector);
    }

    if (failed_teams[getTeam()] > 0)
    {
        Timing::initialiseTiming();
        (*(getLoadCheckpointCallback()))(reload_team);
    }
}

void get_failed_teams_and_ranks(std::vector<int> *failed_ranks, std::unordered_map<int, int> *failed_teams, MPI_Comm shrinked_comm)
{
    MPI_Group group_world, group_world_shrinked, group_failed;
    int world_size, failed_size;

    PMPI_Comm_group(getWorldComm(), &group_world);
    PMPI_Comm_group(shrinked_comm, &group_world_shrinked);
    PMPI_Group_difference(group_world, group_world_shrinked, &group_failed);
    MPI_Group_size(group_world, &world_size);
    MPI_Group_size(group_failed, &failed_size);

    for (int i = 0; i < failed_size; i++)
    {
        int diff_rank;
        PMPI_Group_translate_ranks(group_failed, 1, &i, group_world, &diff_rank);
        failed_ranks->push_back(diff_rank);
        int current_team = mapRankToTeamNumber(diff_rank);
        std::cout << "Failed Rank:" << diff_rank << " failed Team " << current_team << std::endl;
        //(*failed_teams)[current_team]++;
        (*failed_teams)[current_team] = (failed_teams->at(current_team)) + 1;
    }
}

void get_failed_proc_type(std::unordered_map<int, int> *failed_teams, int *failed_normal, int *failed_spares)
{
    for (auto const &i : *failed_teams)
    {
        auto key = i.first;
        auto val = i.second;

        if (key == getNumberOfTeams())
        {
            std::cout << "Team: " << key << " Failed: " << val << std::endl;
            *failed_spares += val;
        }
        else
        {
            std::cout << "Team: " << key << " Failed: " << val << std::endl;
            *failed_normal += val;
        }
    }
}

int get_reload_team(std::unordered_map<int, int> *failed_teams)
{
    for (int i = 0; i <= getNumberOfTeams(); i++)
    {
        if ((*failed_teams)[i] == 0)
        {
            std::cout << "Reload Team: " << i << std::endl;
            if (i == getNumberOfTeams())
            {
                MPI_Abort(MPI_COMM_WORLD, MPI_ERR_INTERN);
            }
            return i;
        }
    }
    return 0;
}