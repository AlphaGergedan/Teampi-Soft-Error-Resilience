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
    if( MPIX_ERR_PROC_FAILED != eclass ) {
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
    MPI_Comm comm = *pcomm, newcomm;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams;
    int *ranks_failed, *ranks_comm;
    std::set<int> set_failed_teams, set_current_failed_teams;

    std::cout << "Errorhandler kill_team_errh invoked on " << rank_team  << " of team: " << team << std::endl;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        MPI_Abort(comm, err);
    }

    num_teams = getNumberOfTeams();
    size_team = getTeamSize();
    rank_team = getTeamRank();
    team = getTeam();

    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_failed);
    MPI_Group_size(group_failed, &num_failed_procs);
    MPI_Comm_group(comm, &group_comm);

    ranks_comm = new int[num_failed_procs];
    ranks_failed = new int[num_failed_procs];

    for(int i = 0; i < num_failed_procs; i++) ranks_failed[i] = i;

    MPI_Group_translate_ranks(group_failed, num_failed_procs, ranks_failed,
                              group_comm, ranks_comm);


    //ranks_comm now contains the ranks of failed processes in MPI_COMM_WORLD
    /* TODOS:
    * 1. Check wich teams conain failed procs
    * 2. Each team that contains one terminates each process belonging to it
    * 3. Should there be no team left MPI_Abort
    * 4. Clean TeaMPI data so that it knows how many teams are still working and shrink MPI_COMM_WORLD
    * 5. Profit???
    */
    
   for(int i = 0; i < num_failed_procs; i++){
       int failed_team = mapRankToTeamNumber(ranks_comm[i]);
       set_failed_teams.insert(failed_team);
       set_current_failed_teams.insert(failed_team);
   }

   if(set_failed_teams.size() == num_teams) MPI_Abort(MPI_COMM_WORLD, MPIX_ERR_PROC_FAILED);

   //So here I create new failed procs
   if(1 == set_current_failed_teams.count(team)){
       //TODO callback cleanup
       std::exit(1);
   }

   
    MPIX_Comm_shrink(comm, &newcomm);
     setLibComm(newcomm);
    set_current_failed_teams.clear();


   
}

void kill_team_errh_comm_team(MPI_Comm* pcomm, int* perr, ...){
    MPI_Group group_failed, group_comm;
    int err = *perr;
    MPI_Comm comm = *pcomm, newcomm;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams, len;
    int *ranks_failed, *ranks_comm;
    std::set<int> set_failed_teams, set_current_failed_teams;
    char errstr[MPI_MAX_ERROR_STRING];

    num_teams = getNumberOfTeams();
    size_team = getTeamSize();
    rank_team = getTeamRank();
    team = getTeam();

    MPI_Error_string(err, errstr, &len);

    std::cout << "Errorhandler kill_team_errh_comm_team invoked on " << rank_team  << " of team: " << team << ", Error: " << errstr << std::endl;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        MPI_Abort(comm, err);
    }

    

    MPIX_Comm_revoke(*pcomm);
    std::cout << "Process " << rank_team << " exiting" << std::endl << boost::stacktrace::stacktrace() << std::endl;;

    std::exit(0);


}
