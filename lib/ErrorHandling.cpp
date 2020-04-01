#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include "ErrorHandling.h"
#include "Rank.h"

//Debugging only
//#include <boost/stacktrace.hpp>

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
    MPI_Comm comm = *pcomm, comm_shrinked;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams, name_len;
    int size_comm, size_shrinked_comm;
    int *ranks_failed, *ranks_comm;
    int rank_old, rank_new;

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
    //1. Shrink the input comm
    MPIX_Comm_shrink(comm, &comm_shrinked);
    MPI_Comm_size(comm_shrinked, &size_shrinked_comm);
    MPI_Comm_size(comm, &size_comm);
    num_failed_procs = size_comm - size_shrinked_comm; /* number of deads */
    

    std::cout << "Size of shrinked comm: " << size_shrinked_comm << std::endl;

    //2. Make sure that we handle failures ourselves
    MPI_Comm_set_errhandler(comm_shrinked, MPI_ERRORS_RETURN);

    //3. Check that failed team is completly dead
    if(size_shrinked_comm % num_teams != 0){
        std::cout << "There still are some living processes from a failed team" << std::endl;
         goto redo;
    }

    
    int flag, flag_result;
    flag = flag_result = 1;
    MPIX_Comm_agree(comm_shrinked, &flag);
    if(flag != flag_result){
        std::cout << "Agreement on flag failed" << std::endl;
        goto redo;
    }  

    PMPI_Comm_rank(comm, &rank_old);
    PMPI_Comm_rank(comm_shrinked, &rank_new);

    std::cout << "kill_team_errh finished on " << rank_team << " of team " << team 
             << ", now has global rank: " << rank_new << " was before: " << rank_old << std::endl;

    
    MPI_Comm_set_errhandler(comm_shrinked, *getWorldErrhandler());
    setLibComm(comm_shrinked);
    
}

    

void kill_team_errh_comm_world_old(MPI_Comm* pcomm, int* perr, ...){
    MPI_Group group_failed, group_comm;
    int err = *perr;
    MPI_Comm comm = *pcomm, newcomm;
    int num_failed_procs, eclass, size_team, rank_team, team, num_teams, name_len;
    int *ranks_failed, *ranks_comm;
    std::set<int> set_failed_teams, set_current_failed_teams;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass) {
        MPI_Abort(comm, err);
    }

    num_teams = getNumberOfTeams();
    size_team = getTeamSize();
    rank_team = getTeamRank();
    team = getTeam();
   

    std::cout << "Errorhandler kill_team_errh_comm_world invoked on " << rank_team  << " of team: " << team  << std::endl;

    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_failed);
    MPI_Group_size(group_failed, &num_failed_procs);
    MPI_Comm_group(comm, &group_comm);

    ranks_comm = new int[num_failed_procs];
    ranks_failed = new int[num_failed_procs];

    for(int i = 0; i < num_failed_procs; i++) ranks_failed[i] = i;

    MPI_Group_translate_ranks(group_failed, num_failed_procs, ranks_failed,
                              group_comm, ranks_comm);

    std::cout << "Number of failed procs: " << num_failed_procs << std::endl;


    //ranks_comm now contains the ranks of failed processes in MPI_COMM_WORLD
    /* TODOS:
    * 1. Check wich teams conain failed procs
    * 2. Each team that contains one terminates each process belonging to it
    * 3. Should there be no team left MPI_Abort
    * 4. Clean TeaMPI data so that it knows how many teams are still working and shrink MPI_COMM_WORLD
    * 5. Profit???
    */
    
   std::string failed_procs = "Team: " + std::to_string(team) + "Rank: " + std::to_string(rank_team) + " recognized: ";
   
   for(int i = 0; i < num_failed_procs; i++){
       failed_procs += std:: to_string(ranks_comm[i]) + " ";

   }
    std::cout << failed_procs << " as failed " << std::endl;



   for(int i = 0; i < num_failed_procs; i++){
       int failed_team = mapRankToTeamNumber(ranks_comm[i]);
       std::cout << "Team: " << team << " Rank: " << rank_team << " recognized team: " << failed_team << " as failed" << std::endl;
       set_failed_teams.insert(failed_team);
       set_current_failed_teams.insert(failed_team);
   }

   if(set_failed_teams.size() == num_teams) MPI_Abort(MPI_COMM_WORLD, MPIX_ERR_PROC_FAILED);

    std::cout << "0: "<< team << " " << rank_team << std::endl;

   //So here I create new failed procs
   if(1 == set_current_failed_teams.count(team)){
        //TODO callback cleanup
        std::cout << "Exiting: "<< team << " " << rank_team << std::endl;
        std::exit(1);
   }

    std::cout << "1: "<< team << " " << rank_team << std::endl;
   
    MPIX_Comm_shrink(comm, &newcomm);
     setLibComm(newcomm);
    set_current_failed_teams.clear();

    std::cout << "2: "<< team << " " << rank_team << std::endl;
    
    std::string failed = "";
    for (const auto &s : set_failed_teams)
    {
        failed += s;
    }
    std::cout << failed << std::endl;
    std::cout << "Retunrning from errhandler: " << rank_team  << " of team: " << team << std::endl;


   
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

    verbose_errh(pcomm, perr);

    std::cout << "Errorhandler kill_team_errh_comm_team invoked on " << rank_team  << " of team: " << team << ", Error: " << errstr << std::endl;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass && MPIX_ERR_REVOKED != eclass) {
        MPI_Abort(comm, err);
    }

    

    MPIX_Comm_revoke(*pcomm);
    std::cout << "Process " << rank_team << " exiting" << std::endl;

    std::exit(0);

    std::cout << "test wtf" << std::endl;

}
