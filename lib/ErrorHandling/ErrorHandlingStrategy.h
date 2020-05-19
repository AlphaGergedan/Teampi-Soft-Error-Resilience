#ifndef ERRORHANDLINGSTRATEGY_H_
#define ERRORHANDLINGSTRATEGY_H_

#include <mpi.h>

class ErrorHandlingStrategy{
    public:
        MPI_Errhandler getErrhandlerTeam(){
            return teamErrorhandler;
        };

        MPI_Errhandler getErrhandlerWorld(){
            return worldErrorhandler;
        };

        ErrorHandlingStrategy();

        ~ErrorHandlingStrategy(){
            MPI_Errhandler_free(&teamErrorhandler);
            MPI_Errhandler_free(&worldErrorhandler);
        };

    protected:
        MPI_Errhandler teamErrorhandler;
        MPI_Errhandler worldErrorhandler;

};


#endif