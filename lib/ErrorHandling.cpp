#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include "ErrorHandling.h"
#include "Rank.h"

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
    if (MPIX_ERR_PROC_FAILED != eclass)
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
    MPI_Group group_failed, group_comm;
    int err = *perr;
    MPI_Comm comm = *pcomm, comm_shrinked, new_lib_comm;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams, name_len;
    int size_comm, size_shrinked_comm;
    int *ranks_failed, *ranks_comm;
    int rank_old, rank_new;
    int flag, flag_result;
    flag = flag_result = 1;

    PMPI_Error_class(err, &eclass);
    if (MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass)
    {
        MPI_Abort(comm, err);
    }

    num_teams = getNumberOfTeams();
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
    num_failed_procs = size_comm - size_shrinked_comm; /* number of deads */

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
    PMPI_Comm_rank(comm_shrinked, &rank_new);

    setNumberOfTeams(size_shrinked_comm / size_team);
    setTeam(rank_new / size_team);

    std::cout << "kill_team_errh finished on " << rank_team << " of team " << team << " now team: " << rank_new / size_team << " "
              << ", now has global rank: " << rank_new << " was before: " << rank_old << std::endl;
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
        MPI_Abort(comm, err);
    }

    rank_team = getTeamRank();
    team = getTeam();

    std::cout << "Errorhandler respawn_proc_errh_comm_world invoked on " << rank_team << " of team: " << team << std::endl;
    respawn_proc_recreate_comm_world(comm);
}

void respawn_proc_recreate_comm_world(MPI_Comm comm)
{
    MPI_Group group_failed, group_comm;
    MPI_Comm comm_world_shrinked, new_world_comm, intercomm, merged_comm, new_comm_team;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams, name_len;
    int size_comm_world, size_comm_world_shrinked;
    int *ranks_failed, *ranks_comm;
    int rank_old, rank_new;
    int flag, flag_result;
    int teamCommRevoked = 0;
    int error;
    bool failedTeam = false;
    flag = flag_result = 1;
    std::set<int> failed_teams;

redo:
    //if new spawn
    if (comm == MPI_COMM_NULL)
    {
        //Get parent and receive new Rank that will be assigned later on
        PMPI_Comm_get_parent(&intercomm);
        comm_world_shrinked = MPI_COMM_WORLD;
        PMPI_Barrier(comm_world_shrinked);
        std::cout << "plop" << std::endl;
        PMPI_Recv(&rank_old, 1, MPI_INT, 0, 1, intercomm, MPI_STATUS_IGNORE);
        std::cout << "Received new rank: " << rank_new << std::endl;
        failedTeam = true;
    }
    else
    {
        //Check Team Comm and set failedTeam correctly
        PMPIX_Comm_agree(getTeamComm(MPI_COMM_WORLD), &flag);
        if (flag != flag_result)
            failedTeam = true;

        
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
        PMPI_Comm_rank(comm, &rank_old);
        PMPI_Comm_rank(comm_world_shrinked, &rank_new);

        MPI_Group group_world, group_world_shrinked, group_failed;
        int diff_rank;

        if(rank_new == 0){
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
            if (rank_new == 0){
                //Send those ranks to new spawn
                PMPI_Send(&diff_rank, 1, MPI_INT, i, 1, intercomm);
            }
            //Save teams that contain failed ranks
            failed_teams.insert(mapRankToTeamNumber(diff_rank));
        }
        PMPI_Group_free(&group_world);
        PMPI_Group_free(&group_world_shrinked);
        PMPI_Group_free(&group_failed);
    }

    //Merge intercomm to create new comm world
    std::cout << "Merging Intercomm" << std::endl;
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
    std::cout << "Reassigin Ranks" << std::endl;
    std::cout << "Size merged comm " << size_merged_comm << std::endl;
    error = PMPI_Comm_split(merged_comm, 1, rank_new, &new_world_comm);
 
    flag = (MPI_SUCCESS==error);
    MPIX_Comm_agree(merged_comm, &flag);
    MPI_Comm_free(&merged_comm);
    if( !flag ) {
        if( MPI_SUCCESS == error ) {
            MPI_Comm_free(&new_world_comm);
        }
        goto redo;
    }

    int size_new_world_comm;
    PMPI_Comm_size(new_world_comm, &size_new_world_comm);
    if (MPI_COMM_NULL != comm)
    {
        //assert(size_new_world_comm == size_comm_world);
        std::cout << "size new world: " << size_new_world_comm << " " << size_comm_world <<  std::endl;;
    }

    int team_size = size_new_world_comm / getNumberOfTeams();
    int color = rank_new / team_size;

    if (MPI_COMM_NULL != comm)
    {
        //assert(teamSize == getTeamSize());
        //assert(rank_new == getWorldRank());
        //assert(color = mapWorldToTeamRank(rank_new));
        if(rank_new == 0){
            std::cout << "teamSize: " << team_size << " " << getTeamSize() << std::endl;
            std::cout << "rankNew: " << rank_new << " " << getWorldRank() << std::endl;
            std::cout << "color: " << color << " " << mapRankToTeamNumber(rank_new) << std::endl;
        }
    }

    //Recreate the team-communicators
    PMPI_Comm_split(new_world_comm, color, rank_old, &new_comm_team);

    MPI_Errhandler errh_world, errh_team;
    PMPI_Comm_create_errhandler(respawn_proc_errh_comm_world, &errh_world);
    PMPI_Comm_set_errhandler(new_world_comm, errh_world);

    PMPI_Comm_create_errhandler(respawn_proc_errh_comm_team, &errh_team);
    PMPI_Comm_set_errhandler(new_comm_team, errh_team);


    //set world comm and team comm to new working comms
    setWorldComm(new_world_comm);
    setTeamComm(new_comm_team);

    // find first team comm that did not suffer from a failure and let it write a checkpoint
    int checkpoint_team;
    for(checkpoint_team = 0; checkpoint_team < getNumberOfTeams(); checkpoint_team++){
        if(failed_teams.count(checkpoint_team) == 0) break;
    }
    if(getTeam() == checkpoint_team) (*getCreateCheckpointCallback())();
    PMPI_Barrier(new_world_comm);
    //only teams with failures load checkpoints
    if (failedTeam)
    {
        (*getLoadCheckpointCallback())();
    }
}
