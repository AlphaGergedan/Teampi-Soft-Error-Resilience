#include <mpi.h>
#include <signal.h>
#include <unistd.h>
#include "../../lib/teaMPI.h"
#include <iostream>

#include <csetjmp>
#define MESSAGE_LENGTH 4096


int main(int argc, char *argv[])
{

    MPI_Init(&argc, &argv);
    int rank, size, team;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    team = TMPI_GetTeamNumber();

    int message[MESSAGE_LENGTH];

    for(int i =0 ; i < MESSAGE_LENGTH; i++){
        if(rank == 0) message[i] = i;
        if(rank == 1)message[i] = 0;
    }
    
    
    for (int i = 0; i <= 2; i++)
    {
            if (rank == 0 && team == 0 && i == 1 ){
                std::cout << "Sigkill" << std::endl;
                raise(SIGKILL);
            }
            if(rank == 0){ 
                MPI_Send(&message, MESSAGE_LENGTH, MPI_INT, size-1, 0, MPI_COMM_WORLD);
                std::cout << "Sent: Team " << team << std::endl;
            }
       
        if(rank == size-1) MPI_Recv(&message, MESSAGE_LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        
    }

    
    MPI_Finalize();
    return 0;
}
