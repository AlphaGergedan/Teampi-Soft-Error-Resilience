#include <mpi.h>
#include <signal.h>
#include "../../lib/teaMPI.h"
#include <iostream>
#define MESSAGE_LENGTH 4096

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size, team;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    team = TMPI_GetTeamNumber();
    
    int message[MESSAGE_LENGTH];
    
    
    if(rank == 0){
        for(int i = 0; i < MESSAGE_LENGTH; i++){
            message[i] = i;
        }
        std::cout << "sending: " << team << std::endl;
        MPI_Send(&message, MESSAGE_LENGTH, MPI_INT, 1, 0, MPI_COMM_WORLD );
    }
    if(rank == 1){
        if(team == 1) raise(SIGKILL);
        std::cout << "receiving: " << team << std::endl;
        MPI_Recv(&message, MESSAGE_LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
    
    MPI_Finalize();
    std::cout << "Finalized: " << rank << "  " << team;
    return 0;
 }
