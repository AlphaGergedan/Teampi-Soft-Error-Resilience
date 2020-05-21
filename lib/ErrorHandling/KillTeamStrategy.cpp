#include <mpi.h>
#include <mpi-ext.h>
#include <iostream>

#include "../Rank.h"
#include "KillTeamStrategy.h"

/*
*The Errorhandler for MPI_COMM_WORLD when using the KILL_FAILED_TEAMS statergy.
*This Errorhandler shrinks MPI_COMM_WORLD / removes failed processes
*/
void kill_team_errh_comm_world(MPI_Comm *pcomm, int *perr, ...)
{
    int err = *perr;
    MPI_Comm comm = *pcomm, comm_shrinked, new_lib_comm;
    int eclass, size_team, rank_team, team;
    int size_comm, size_shrinked_comm;
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

    //std::cout << boost::stacktrace::stacktrace();

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



