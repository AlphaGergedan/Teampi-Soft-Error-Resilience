#include <mpi.h>
#include <signal.h>
#include "../../lib/teaMPI.h"
#include <iostream>
#define MESSAGE_LENGTH 4096

bool fail = true;


void createCheckpoint(){
    fail = false;
    std::cout << "Created Checkpoint" << std::endl;
}

void loadedCheckpoint(){
    fail = false;
    std::cout << "Loaded Chekpoint" << std::endl;
}

int main(int argc, char *argv[])
{
    std::function<void(void)> create(createCheckpoint);
    std::function<void(void)> load(loadedCheckpoint);
    TMPI_SetCreateCheckpointCallback(&create);
    TMPI_SetLoadCheckpointCallback(&load);
    std::cout << "Init MPI" << std::endl;
    MPI_Init(&argc, &argv);
    std::cout << "Finished Init" << std::endl;
    int rank, size, team;
    MPI_Status status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    team = TMPI_GetTeamNumber();

    int message[MESSAGE_LENGTH];

    for (int i = 0; i <= 1024; i++)
    {
        if (rank == 0)
        {
            for (int i = 0; i < MESSAGE_LENGTH; i++)
            {
                message[i] = i;
            }
            //std::cout << "sending: " << team << std::endl;
            MPI_Send(&message, MESSAGE_LENGTH, MPI_INT, 1, 0, MPI_COMM_WORLD);
            //std::cout << "finished sending " << team << std::endl;
        }
        if (rank == 1)
        {
            if (team == 0 && i == 512 && fail){
                std::cout << "Sigkill" << std::endl;
                raise(SIGKILL);
            }
                
            //std::cout << "receiving: " << team << std::endl;
            MPI_Recv(&message, MESSAGE_LENGTH, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            //std::cout << "finished receiving " << team << std::endl;
        }

        MPI_Bcast(nullptr, 0, MPI_INT, 0, MPI_COMM_SELF);
    }

    std::cout << "Finalizing: " << rank << "  " << team << std::endl;
    MPI_Finalize();
    std::cout << "Finalized: " << rank << "  " << team << std::endl;
    return 0;
}
