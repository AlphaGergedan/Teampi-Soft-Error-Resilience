#include <mpi.h>
#include <mpi-ext.h>
#include <iostream>
//#include <boost/stacktrace.hpp>
#include <set>

#include "../Rank.h"
#include "RespawnProcStrategy.h"
#include "../Timing.h"

//Based on example from 2018 tutorial found on ULFM website
void respawn_proc_errh(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm;
    int eclass, rank_team, team;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        std::cout << "Aborting: Unexpected Error" << eclass << std::endl;
        PMPI_Abort(comm, err);
    }

    rank_team = getTeamRank();
    team = getTeam();
    std::cout << "Errorhandler respawn_proc_errh invoked on " << rank_team << " of team: " << team << std::endl;
    PMPIX_Comm_revoke(getWorldComm());
    PMPIX_Comm_revoke(getLibComm());
    PMPIX_Comm_revoke(getTeamComm(MPI_COMM_WORLD));
    PMPIX_Comm_revoke(getTeamInterComm());
    
    //std::cout << boost::stacktrace::stacktrace() << " Team: " << team  << " Rank: " << rank_team << std::endl << std::endl;

    respawn_proc_recreate_world(false);
}

void respawn_proc_recreate_world(bool newSpawn)
{
    MPI_Comm comm_world_shrinked, new_comm_world, intercomm, merged_comm, new_comm_team;
    int num_failed_procs;
    int size_comm_world, size_comm_world_shrinked;
    int rank_in_new_world, rank_in_shrinked_world;
    int flag, flag_result;
    bool failed_team = false;
    int teamCommRevoked = 0;
    int error;
    int checkpoint_team;
    flag = flag_result = 1;
    
    
redo:
    //if new spawn
    if (newSpawn)
    {
        //Get parent and receive new Rank that will be assigned later on
        PMPI_Comm_get_parent(&intercomm);
        comm_world_shrinked = MPI_COMM_WORLD;
        PMPI_Barrier(comm_world_shrinked);
        PMPI_Recv(&rank_in_new_world, 1, MPI_INT, 0, 1, intercomm, MPI_STATUS_IGNORE);
        PMPI_Recv(&checkpoint_team, 1, MPI_INT, 0, 1, intercomm, MPI_STATUS_IGNORE);
        std::cout << "Received new rank: " << rank_in_new_world << std::endl;
        failed_team = true;

    }
    else
    {
        std::set<int> failed_teams;
        //Processes here are survivors
        flag = MPI_SUCCESS;
        PMPIX_Comm_is_revoked(getTeamComm(MPI_COMM_WORLD), &teamCommRevoked);
   
        //Remove all failed procs from comm world --> comm_world_shrinked
        PMPIX_Comm_shrink(getWorldComm(), &comm_world_shrinked);

        PMPI_Comm_size(getWorldComm(), &size_comm_world);
        PMPI_Comm_size(comm_world_shrinked, &size_comm_world_shrinked);

        num_failed_procs = size_comm_world - size_comm_world_shrinked;

        //Handle failures ourselves
        PMPI_Comm_set_errhandler(getWorldComm(), MPI_ERRORS_RETURN);
        PMPI_Comm_set_errhandler(getTeamComm(MPI_COMM_WORLD), MPI_ERRORS_RETURN);

        //Respwan Processes
        char **argValues = *getArgValues();

        error = PMPI_Comm_spawn(argValues[0], &argValues[1], num_failed_procs, MPI_INFO_NULL, 0, comm_world_shrinked,
                                &intercomm, MPI_ERRCODES_IGNORE);

        //Check that spawn was sucessfull
        flag = (MPI_SUCCESS == error);
        PMPIX_Comm_agree(comm_world_shrinked, &flag);
        if (!flag)
        {
            if (MPI_SUCCESS == error)
            {
                PMPIX_Comm_revoke(intercomm);
                PMPI_Comm_free(&intercomm);
            }
            PMPI_Comm_free(&comm_world_shrinked);
            std::cout << "Comm spawn failed, retrying" << std::endl;
            goto redo;
        }

        //Ranks in original comm_world and shrunken comm_world
        PMPI_Comm_rank(getWorldComm(), &rank_in_new_world);
        PMPI_Comm_rank(comm_world_shrinked, &rank_in_shrinked_world);

        MPI_Group group_world, group_world_shrinked, group_failed;
        int diff_rank;

        if(rank_in_shrinked_world == 0){
            std::cout << size_comm_world_shrinked << " " << num_failed_procs << std::endl;
        }

        //Calculate failed procs
        PMPI_Comm_group(getWorldComm(), &group_world);
        PMPI_Comm_group(comm_world_shrinked, &group_world_shrinked);

        //Processes that are in group_world but not group_world_shrinked have failed
        PMPI_Group_difference(group_world, group_world_shrinked, &group_failed);

        for (int i = 0; i < num_failed_procs; i++)
        {
            //Get Ranks of procs in comm_world that have failed in comm_world
            PMPI_Group_translate_ranks(group_failed, 1, &i, group_world, &diff_rank);
            if (rank_in_shrinked_world == 0){
                //Send those ranks to new spawn
                PMPI_Send(&diff_rank, 1, MPI_INT, i, 1, intercomm);
            }
            //Save teams that contain failed ranks
            int currentTeam = mapRankToTeamNumber(diff_rank);
            failed_teams.insert(currentTeam);
            if(currentTeam == getTeam()) failed_team = true;
        }

    // find first team comm that did not suffer from a failure and let it write a checkpoint
    //This does not work like this!!
    
        for(checkpoint_team = 0; checkpoint_team <= getNumberOfTeams(); checkpoint_team++){
            if(failed_teams.count(checkpoint_team) == 0) break;
            if(checkpoint_team == getNumberOfTeams()){
                std::cerr << "No team without failures, aborting!!" << std::endl;
                MPI_Abort(new_comm_world, MPIX_ERR_PROC_FAILED);
            }
        }

        std::cout << "Checkpoint Team: " << checkpoint_team << std::endl;
        if(rank_in_shrinked_world == 0){
            for(int i = 0; i < num_failed_procs; i++){
                PMPI_Group_translate_ranks(group_failed, 1, &i, group_world, &diff_rank);
                PMPI_Send(&checkpoint_team, 1, MPI_INT, i, 1, intercomm);
            }
        }
        
        PMPI_Group_free(&group_world);
        PMPI_Group_free(&group_world_shrinked);
        PMPI_Group_free(&group_failed);

    }

    //Merge intercomm to create new comm world
    //std::cout << "Merging Intercomm" << std::endl;
    error = PMPI_Intercomm_merge(intercomm, 1, &merged_comm);
    flag_result = flag = (MPI_SUCCESS == error);
    PMPIX_Comm_agree(comm_world_shrinked, &flag);
    if (MPI_COMM_WORLD != comm_world_shrinked)
        PMPI_Comm_free(&comm_world_shrinked);
    PMPIX_Comm_agree(intercomm, &flag_result);
    PMPI_Comm_free(&intercomm);
    if (!(flag && flag_result))
    {
        if (MPI_SUCCESS == error)
        {
            PMPI_Comm_free(&merged_comm);
        }
        goto redo;
    }

    //Reassign the ranks in correct order 
    int size_merged_comm;
    PMPI_Comm_size(merged_comm, &size_merged_comm);
    //std::cout << "Reassigin Ranks" << std::endl;
    //std::cout << "Size merged comm " << size_merged_comm << std::endl;
    error = PMPI_Comm_split(merged_comm, 1, rank_in_new_world, &new_comm_world);
 
    flag = (MPI_SUCCESS==error);
    MPIX_Comm_agree(merged_comm, &flag);
    PMPI_Comm_free(&merged_comm);
    if( !flag ) {
        if( MPI_SUCCESS == error ) {
            PMPI_Comm_free(&new_comm_world);
        }
        goto redo;
    }

    int size_new_comm_world;
    PMPI_Comm_size(new_comm_world, &size_new_comm_world);

    int team_size = size_new_comm_world / getNumberOfTeams();
    int color = rank_in_new_world / team_size;

    //Recreate the team-communicators
    //std::cout << "Color: " << color << " Rank_old: " << rank_old << std::endl;
    PMPI_Comm_split(new_comm_world, color, rank_in_new_world, &new_comm_team);
    MPI_Comm new_lib_comm;
    PMPI_Comm_dup(new_comm_world, &new_lib_comm);

    MPI_Errhandler errh_world, errh_team;
    PMPI_Comm_create_errhandler(respawn_proc_errh, &errh_world);
    PMPI_Comm_set_errhandler(new_comm_world, errh_world);
    PMPI_Comm_set_errhandler(new_lib_comm, errh_world);

    PMPI_Comm_create_errhandler(respawn_proc_errh, &errh_team);
    PMPI_Comm_set_errhandler(new_comm_team, errh_team);

    //set world comm and team comm to new working comms
    setWorldComm(new_comm_world);
    setTeamComm(new_comm_team);
    setLibComm(new_lib_comm);

    int team_rank;
    
    PMPI_Comm_rank(new_comm_team, &team_rank);
    PMPI_Comm_size(new_comm_team, &team_size);
    setTeamRank(team_rank);
    setTeam(color);
    
    std::cout << "Finished repair: Team: " << color << " Rank: " << team_rank << " Global Rank: " << rank_in_new_world  << std::endl;
    
    //TODO this needs to be fixed
    std::vector<int> failed_team_vector;
    PMPI_Barrier(new_comm_world);
    if(getTeam() == checkpoint_team && !failed_team) (*getCreateCheckpointCallback())(failed_team_vector);
    
    //only teams with failures load checkpoints
    if(failed_team && !newSpawn){
        Timing::initialiseTiming();
        (*getLoadCheckpointCallback())(false);
        
    } 
        
    
}
