#include <mpi.h>
#include <signal.h>
#include <unistd.h>
#include "../../lib/teaMPI.h"
#include <iostream>

#include <csetjmp>
#define MESSAGE_LENGTH 4096

int fail = 0;
jmp_buf buffer;


void createCheckpoint(std::vector<int> failed){
    fail++;
    std::cout << "Created Checkpoint" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
}

void loadCheckpoint(bool loadFrom){
    MPI_Barrier(MPI_COMM_WORLD);
    fail++;
    std::cout << "Loaded Chekpoint" << std::endl;
    longjmp(buffer, 1);
}

int main(int argc, char *argv[])
{
    std::function<void(std::vector<int>)> create(createCheckpoint);
    std::function<void(int)> load(loadCheckpoint);
    TMPI_SetCreateCheckpointCallback(&create);
    TMPI_SetLoadCheckpointCallback(&load);
    TMPI_SetErrorHandlingStrategy(TMPI_WarmSpareErrorHandler);


    if(setjmp(buffer) == 0) MPI_Init(&argc, &argv);
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
            if (rank == 0 && team == 0 && i == 1 && fail < 1){
                std::cout << "Sigkill" << std::endl;
                raise(SIGKILL);
            }
            if(rank == 0){ 
                MPI_Send(&message, MESSAGE_LENGTH, MPI_INT, size-1, 0, MPI_COMM_WORLD);
                std::cout << "Sent: Team " << team << std::endl;
            }
       
        if(rank == size-1) MPI_Recv(&message, MESSAGE_LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        //Hearbeat
        MPI_Allreduce(nullptr, nullptr, 0, MPI_INT, MPI_MIN, MPI_COMM_SELF);
        
    }

    
    MPI_Finalize();
    return 0;
}
