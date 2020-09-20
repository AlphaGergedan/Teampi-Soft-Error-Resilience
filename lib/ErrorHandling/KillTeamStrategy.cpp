#include <mpi.h>
#include <mpi-ext.h>
#include <iostream>

//#include <boost/stacktrace.hpp>

#include "../Rank.h"
#include "KillTeamStrategy.h"

/*
*The Errorhandler for MPI_COMM_WORLD when using the KILL_FAILED_TEAMS statergy.
*This Errorhandler shrinks MPI_COMM_WORLD / removes failed processes
*/
void kill_team_errh_comm_world(MPI_Comm *pcomm, int *perr, ...)
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

    PMPIX_Comm_revoke(getTeamComm(MPI_COMM_WORLD));
    PMPIX_Comm_revoke(getWorldComm());

    //std::cout << boost::stacktrace::stacktrace();
    std::cout << "Errorhandler kill_team_errh_comm_world invoked on " << rank_team << " of team: " << team << std::endl;

    kill_team_recreate_world(false);

}


void kill_team_recreate_world(bool isSpareRank){
    MPI_Comm comm_shrinked, new_lib_comm, team_shrinked;
    int size_shrinked_comm, size_shrinked_team;
    int rank_old, rank_in_shrinked_world;
    int flag, flag_result;
    flag = flag_result = 1;

    
    //Check if team contains failures and exit
    PMPIX_Comm_shrink(getTeamComm(MPI_COMM_WORLD), &team_shrinked);
    PMPI_Comm_size(team_shrinked, &size_shrinked_team);
    
    if (size_shrinked_team < getTeamSize())
    {
        std::cout << "Team contains failed processes exiting" << std::endl;
        std::exit(0);
    }

redo:
    //Shrink the world comm
    PMPIX_Comm_shrink(getWorldComm(), &comm_shrinked);
    PMPI_Comm_size(comm_shrinked, &size_shrinked_comm);

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
    if (size_shrinked_comm % getTeamSize() != 0)
    {
        std::cout << "There still are some living processes from a failed team" << std::endl;
        goto redo;
    }

    
    PMPIX_Comm_agree(comm_shrinked, &flag);
    if (flag != flag_result)
    {
        std::cout << "Agreement on flag failed" << std::endl;
        goto redo;
    }

    //Rebuild world
    PMPI_Comm_rank(getWorldComm(), &rank_old);
    PMPI_Comm_rank(comm_shrinked, &rank_in_shrinked_world);

    setNumberOfTeams(size_shrinked_comm / getTeamSize());
    setTeam(rank_in_shrinked_world / getTeamSize());

    std::cout << "kill_team_errh finished on " << getTeamRank() << " of team " << getTeam() << " now team: " << rank_in_shrinked_world / getTeamSize() << " "
              << ", now has global rank: " << rank_in_shrinked_world << " was before: " << rank_old << std::endl;
    std::cout << "Number of teams now: " << size_shrinked_comm / getTeamSize() << std::endl;

    PMPI_Comm_set_errhandler(comm_shrinked, *getWorldErrhandler());
    PMPI_Comm_dup(comm_shrinked, &new_lib_comm);
    setWorldComm(comm_shrinked);
    setLibComm(new_lib_comm);

    refreshWorldRank();
}

