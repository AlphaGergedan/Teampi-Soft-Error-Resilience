#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include "ErrorHandling.h"
#include "Rank.h"

#include "Timing.h"

//Debugging only
#include <boost/stacktrace.hpp>

void verbose_errh(MPI_Comm *pcomm, int *perr, ...)
{
    MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, size, nf, len, eclass, team;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass  && MPIX_ERR_REVOKED != eclass)
    {
        MPI_Abort(comm, err);
    }

    size = getTeamSize();
    rank = getTeamRank();
    team = getTeam();

    PMPI_Comm_size(comm, &size);

    PMPIX_Comm_failure_ack(comm);
    PMPIX_Comm_failure_get_acked(comm, &group_f);
    PMPI_Group_size(group_f, &nf);
    PMPI_Error_string(err, errstr, &len);
    printf("Rank %d / %d of Team %d: Notified of error %s. %d found dead: { ",
           rank, size, team, errstr, nf);

    ranks_gf = (int *)malloc(nf * sizeof(int));
    ranks_gc = (int *)malloc(nf * sizeof(int));
    PMPI_Comm_group(comm, &group_c);
    for (i = 0; i < nf; i++)
        ranks_gf[i] = i;
    PMPI_Group_translate_ranks(group_f, nf, ranks_gf,
                               group_c, ranks_gc);
    for (i = 0; i < nf; i++)
        printf("%d ", ranks_gc[i]);
    printf("}\n");
}

/*TODO
*The Errorhandler for MPI_COMM_WORLD when using the KILL_FAILED_TEAMS statergy.
*This Errorhandler shrinks MPI_COMM_WORLD / removes failed processes
*/
void kill_team_errh_comm_world(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm, comm_shrinked, new_lib_comm;
    int eclass, size_team, rank_team, team, name_len;
    int size_comm, size_shrinked_comm;
    int *ranks_failed, *ranks_comm;
    int rank_old, rank_in_shrinked_world;
    int flag, flag_result;
    flag = flag_result = 1;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        MPI_Abort(comm, err);
    }

    size_team = getTeamSize();
    rank_team = getTeamRank();
    team = getTeam();

    std::cout << "Errorhandler kill_team_errh_comm_world invoked on " << rank_team << " of team: " << team << std::endl;

    std::cout << boost::stacktrace::stacktrace();

redo:
    //Make sure own team is safe
    PMPIX_Comm_agree(getTeamComm(MPI_COMM_WORLD), &flag);
    if (flag != flag_result)
    {
        std::cout << "Agreement on flag failed, exiting" << std::endl;
        std::exit(0);
    }

    //Shrink the input comm
    PMPIX_Comm_shrink(comm, &comm_shrinked);
    PMPI_Comm_size(comm_shrinked, &size_shrinked_comm);
    PMPI_Comm_size(comm, &size_comm);

    std::cout << "Size of shrinked comm: " << size_shrinked_comm << std::endl;

    //Make sure that we handle failures ourselves
    PMPI_Comm_set_errhandler(comm_shrinked, MPI_ERRORS_RETURN);

    //Should not be possible but just to be safe
    if (size_shrinked_comm < getTeamSize())
    {
        std::cout << "Failure in last team: Aborting";
        PMPI_Abort(comm_shrinked, MPIX_ERR_PROC_FAILED);
    }

    //Check that failed team is completly dead
    if (size_shrinked_comm % size_team != 0)
    {
        std::cout << "There still are some living processes from a failed team" << std::endl;
        goto redo;
    }

    //
    PMPIX_Comm_agree(comm_shrinked, &flag);
    if (flag != flag_result)
    {
        std::cout << "Agreement on flag failed" << std::endl;
        goto redo;
    }

    PMPI_Comm_rank(comm, &rank_old);
    PMPI_Comm_rank(comm_shrinked, &rank_in_shrinked_world);

    setNumberOfTeams(size_shrinked_comm / size_team);
    setTeam(rank_in_shrinked_world / size_team);

    std::cout << "kill_team_errh finished on " << rank_team << " of team " << team << " now team: " << rank_in_shrinked_world / size_team << " "
              << ", now has global rank: " << rank_in_shrinked_world << " was before: " << rank_old << std::endl;
    std::cout << "Number of teams now: " << size_shrinked_comm / size_team << std::endl;

    PMPI_Comm_set_errhandler(comm_shrinked, *getWorldErrhandler());
    PMPI_Comm_dup(comm_shrinked, &new_lib_comm);
    setWorldComm(comm_shrinked);
    setLibComm(new_lib_comm);

    refreshWorldRank();
    refreshWorldSize();
}

void kill_team_errh_comm_team(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm;
    int eclass, rank_team, team, len;
    char errstr[MPI_MAX_ERROR_STRING];

    rank_team = getTeamRank();
    team = getTeam();

    PMPI_Error_string(err, errstr, &len);

    verbose_errh(pcomm, perr);

    std::cout << "Errorhandler kill_team_errh_comm_team invoked on " << rank_team << " of team: " << team << ", Error: " << errstr << std::endl;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        MPI_Abort(comm, err);
    }

    PMPIX_Comm_revoke(*pcomm);
    std::cout << "Process " << rank_team << " exiting" << std::endl;

    //TODO implement callback to cleanup code

    std::exit(0);
}

void respawn_proc_errh_comm_team(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm;
    int eclass, rank_team, team, len;
    char errstr[MPI_MAX_ERROR_STRING];

    rank_team = getTeamRank();
    team = getTeam();

    PMPI_Error_string(err, errstr, &len);

    verbose_errh(pcomm, perr);

    std::cout << "Errorhandler respawn_proc_errh_comm_team  invoked on: " << rank_team << " of team: " << team << ", Error: " << errstr << std::endl;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        std::cout << "Abort 1" << std::endl;
        MPI_Abort(comm, err);
    }

    PMPIX_Comm_revoke(comm);
    PMPIX_Comm_revoke(getWorldComm());

    respawn_proc_recreate_comm_world(getWorldComm());
}

//Based on example from 2018 tutorial found on ULFM website
void respawn_proc_errh_comm_world(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm;
    int eclass, rank_team, team;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        std::cout << "Abort 2: " << eclass << std::endl;
        MPI_Abort(comm, err);
    }

    rank_team = getTeamRank();
    team = getTeam();
    std::cout << "Errorhandler respawn_proc_errh_comm_world invoked on " << rank_team << " of team: " << team << std::endl;
    PMPIX_Comm_revoke(getWorldComm());
    PMPIX_Comm_revoke(getLibComm());
    PMPIX_Comm_revoke(getTeamComm(MPI_COMM_WORLD));
    
    int flag = MPI_SUCCESS;
    std::cout << boost::stacktrace::stacktrace() << " Team: " << team  << " Rank: " << rank_team << std::endl << std::endl;

    respawn_proc_recreate_comm_world(comm);
}

void respawn_proc_recreate_comm_world(MPI_Comm comm)
{
    MPI_Group group_failed, group_comm;
    MPI_Comm comm_world_shrinked, new_comm_world, intercomm, merged_comm, new_comm_team;
    int num_failed_procs, eclass, size_team, rank_team, num_teams, name_len;
    int size_comm_world, size_comm_world_shrinked;
    int *ranks_failed, *ranks_comm;
    int rank_in_new_world, rank_in_shrinked_world;
    int flag, flag_result;
    bool failed_team;
    int teamCommRevoked = 0;
    int error;
    int checkpoint_team;
    flag = flag_result = 1;
    

    int agree_error;
redo:
    //if new spawn
    if (comm == MPI_COMM_NULL)
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
        //Check Team Comm and set failedTeam correctly
        flag = MPI_SUCCESS;
        PMPIX_Comm_is_revoked(getTeamComm(MPI_COMM_WORLD), &teamCommRevoked);
        PMPIX_Comm_agree(getTeamComm(MPI_COMM_WORLD), &flag);
        std::cout << "Flag: " << flag << std::endl;
   
        //Remove all failed procs from comm world --> comm_world_shrinked
        PMPIX_Comm_shrink(comm, &comm_world_shrinked);

        PMPI_Comm_size(comm, &size_comm_world);
        PMPI_Comm_size(comm_world_shrinked, &size_comm_world_shrinked);

        num_failed_procs = size_comm_world - size_comm_world_shrinked;


        //Handle failures ourselves
        PMPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
        PMPI_Comm_set_errhandler(getTeamComm(MPI_COMM_WORLD), MPI_ERRORS_RETURN);

        //Respwan Processes
        char **argValues = *getArgValues();
        int argCount = getArgCount();

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
        PMPI_Comm_rank(comm, &rank_in_new_world);
        PMPI_Comm_rank(comm_world_shrinked, &rank_in_shrinked_world);

        MPI_Group group_world, group_world_shrinked, group_failed;
        int diff_rank;

        if(rank_in_shrinked_world == 0){
            std::cout << size_comm_world_shrinked << " " << num_failed_procs << std::endl;
        }

        //Calculate failed procs
        PMPI_Comm_group(comm, &group_world);
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
                std::cout << "Abort 3" << std::endl;
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
    PMPI_Comm_create_errhandler(respawn_proc_errh_comm_world, &errh_world);
    PMPI_Comm_set_errhandler(new_comm_world, errh_world);
    PMPI_Comm_set_errhandler(new_lib_comm, errh_world);

    PMPI_Comm_create_errhandler(respawn_proc_errh_comm_team, &errh_team);
    PMPI_Comm_set_errhandler(new_comm_team, errh_team);

    //set world comm and team comm to new working comms
    setWorldComm(new_comm_world);
    setTeamComm(new_comm_team);
    setLibComm(new_lib_comm);

    int team_rank;
    PMPI_Comm_rank(new_comm_team, &team_rank);
    PMPI_Comm_size(new_comm_team, &team_size);
    std::cout << "Finished repair: Team: " << color << " Rank: " << team_rank << " Global Rank: " << rank_in_new_world <<" Size: " << team_size << " Failed Team: " << failed_team << std::endl;
    
    PMPI_Barrier(new_comm_world);
    if(getTeam() == checkpoint_team && !failed_team) (*getCreateCheckpointCallback())();
    
    //only teams with failures load checkpoints
    if (failed_team && comm != MPI_COMM_NULL){
        Timing::initialiseTiming();
        (*getLoadCheckpointCallback())(false);
        
    } 
        
    
    
}
