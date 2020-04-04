#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include "ErrorHandling.h"
#include "Rank.h"

//Debugging only
#include <boost/stacktrace.hpp>

void verbose_errh(MPI_Comm* pcomm, int* perr, ...) {
    MPI_Comm comm = *pcomm;
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, rank, size, nf, len, eclass, team;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass  ) {
        MPI_Abort(comm, err);
    }

    size = getTeamSize();
    rank = getTeamRank();
    team = getTeam();

    MPI_Comm_size(comm, &size);

    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Group_size(group_f, &nf);
    MPI_Error_string(err, errstr, &len);
    printf("Rank %d / %d of Team %d: Notified of error %s. %d found dead: { ",
           rank, size, team, errstr, nf);

    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    MPI_Comm_group(comm, &group_c);
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;
    MPI_Group_translate_ranks(group_f, nf, ranks_gf,
                              group_c, ranks_gc);
    for(i = 0; i < nf; i++)
        printf("%d ", ranks_gc[i]);
    printf("}\n");

}

/*TODO
*The Errorhandler for MPI_COMM_WORLD when using the KILL_FAILED_TEAMS statergy.
*This Errorhandler shrinks MPI_COMM_WORLD / removes failed processes
*/
void kill_team_errh_comm_world(MPI_Comm* pcomm, int* perr, ...){
    MPI_Group group_failed, group_comm;
    int err = *perr;
    MPI_Comm comm = *pcomm, comm_shrinked, new_lib_comm;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams, name_len;
    int size_comm, size_shrinked_comm;
    int *ranks_failed, *ranks_comm;
    int rank_old, rank_new;
    int flag, flag_result;
    flag = flag_result = 1;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass) {
        MPI_Abort(comm, err);
    }

    num_teams = getNumberOfTeams();
    size_team = getTeamSize();
    rank_team = getTeamRank();
    team = getTeam();

    std::cout << "Errorhandler kill_team_errh_comm_world invoked on " << rank_team  << " of team: " << team  << std::endl;

    std::cout << boost::stacktrace::stacktrace();

    redo:
    //Make sure own team is safe
    MPIX_Comm_agree(getTeamComm(MPI_COMM_WORLD), &flag);
    if(flag != flag_result){
        std::cout << "Agreement on flag failed, exiting" << std::endl;
        std::exit(0);
    } 

    //Shrink the input comm
    MPIX_Comm_shrink(comm, &comm_shrinked);
    MPI_Comm_size(comm_shrinked, &size_shrinked_comm);
    MPI_Comm_size(comm, &size_comm);
    num_failed_procs = size_comm - size_shrinked_comm; /* number of deads */
    

    std::cout << "Size of shrinked comm: " << size_shrinked_comm << std::endl;

    //Make sure that we handle failures ourselves
    MPI_Comm_set_errhandler(comm_shrinked, MPI_ERRORS_RETURN);

    //Should not be possible but just to be safe
    if(size_shrinked_comm < getTeamSize()){
        std::cout << "Failure in last team: Aborting";
        MPI_Abort(comm_shrinked, MPIX_ERR_PROC_FAILED);
    }

    //Check that failed team is completly dead
    if(size_shrinked_comm % size_team != 0){
        std::cout << "There still are some living processes from a failed team" << std::endl;
         goto redo;
    }

    
    //
    MPIX_Comm_agree(comm_shrinked, &flag);
    if(flag != flag_result){
        std::cout << "Agreement on flag failed" << std::endl;
        goto redo;
    } 

    PMPI_Comm_rank(comm, &rank_old);
    PMPI_Comm_rank(comm_shrinked, &rank_new);

    setNumberOfTeams(size_shrinked_comm / size_team);
    setTeam(rank_new / size_team);
    

    std::cout << "kill_team_errh finished on " << rank_team << " of team " << team << " now team: " << rank_new/size_team << " " 
             << ", now has global rank: " << rank_new << " was before: " << rank_old << std::endl;
    std::cout << "Number of teams now: " << size_shrinked_comm/size_team << std::endl;

    
    MPI_Comm_set_errhandler(comm_shrinked, *getWorldErrhandler());
    MPI_Comm_dup(comm_shrinked, &new_lib_comm);
    setWorldComm(comm_shrinked);
    setLibComm(new_lib_comm);

    refreshWorldRank();
    refreshWorldSize();
    
}


void kill_team_errh_comm_team(MPI_Comm* pcomm, int* perr, ...){
    int err = *perr;
    MPI_Comm comm = *pcomm;
    int  eclass, rank_team, team, len;
    char errstr[MPI_MAX_ERROR_STRING];

    
    rank_team = getTeamRank();
    team = getTeam();

    MPI_Error_string(err, errstr, &len);

    verbose_errh(pcomm, perr);

    std::cout << "Errorhandler kill_team_errh_comm_team invoked on " << rank_team  << " of team: " << team << ", Error: " << errstr << std::endl;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass) {
        MPI_Abort(comm, err);
    }

    

    MPIX_Comm_revoke(*pcomm);
    std::cout << "Process " << rank_team << " exiting" << std::endl;

    //TODO implement callback to cleanup code

    std::exit(0);

}
