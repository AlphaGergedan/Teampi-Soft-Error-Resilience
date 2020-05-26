#include <mpi.h>
#include <signal.h>
#include <unistd.h>
#include "../../lib/teaMPI.h"
#include <iostream>

#include <csetjmp>
#define MESSAGE_LENGTH 4096

int fail = 0;
jmp_buf buffer;


void createCheckpoint(){
    fail++;
    std::cout << "Created Checkpoint" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
}

void loadCheckpoint(bool newSpawn){
    MPI_Barrier(MPI_COMM_WORLD);
    fail++;
    std::cout << "Loaded Chekpoint" << std::endl;
    if(!newSpawn){
        longjmp(buffer, 1);
    }
}

int main(int argc, char *argv[])
{
    volatile int i = 1;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("PID %d on %s ready for attach\n", getpid(), hostname);
    fflush(stdout);
    while (0 == i) sleep(5);
    std::function<void(void)> create(createCheckpoint);
    std::function<void(bool)> load(loadCheckpoint);
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
    
    std::cout << "Starting Team: " << team << " Rank: " << rank << std::endl;
    for (int i = 0; i <= 2; i++)
    {
        if (rank < size/2)
        {
            for (int i = 0; i < MESSAGE_LENGTH; i++)
            {
                message[i] = i;
            }
            if (team == 0 && i == 1 && fail < 2){
                std::cout << "Sigkill Send" << std::endl;
                raise(SIGKILL);
            }
            //std::cout << "sending: " << team << std::endl;
            if(rank == 0)MPI_Send(&message, MESSAGE_LENGTH, MPI_INT, size-1, 0, MPI_COMM_WORLD);
            std::cout << "Sent: Team " << team << std::endl;
        }
        if (rank >= size/2)
        {
            if (team == 1 && i == 2 && fail < 2){
                std::cout << "Sigkill Recv" << std::endl;
                raise(SIGKILL);
            }
            
            //std::cout << "receiving: " << team << std::endl;
            if(rank == size-1)MPI_Recv(&message, MESSAGE_LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            std::cout << "Received: Team " << team << " " << message[6] << std::endl;
        }

        //MPI_Barrier(MPI_COMM_WORLD);
        MPI_Allreduce(nullptr, nullptr, 0, MPI_INT, MPI_MIN, MPI_COMM_SELF);
        std::cout << "Reduced: Team " << team << std::endl;
        
    }

    std::cout << "Finalizing: Team: " << team << " Rank: " << rank << std::endl;
    MPI_Finalize();
    std::cout << "Finalized: " << team << " Rank " << rank << std::endl;
    return 0;
}
