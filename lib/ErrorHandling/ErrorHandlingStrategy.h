#ifndef ERRORHANDLINGSTRATEGY_H_
#define ERRORHANDLINGSTRATEGY_H_

#include <mpi.h>

class ErrorHandler{
    public:
        MPI_Errhandler getErrhandlerTeam(){
            return teamErrorhandler;
        };

        MPI_Errhandler getErrhandlerWorld(){
            return worldErrorhandler;
        };

        ErrorHandler();

        ~ErrorHandler(){
            MPI_Errhandler_free(&teamErrorhandler);
            MPI_Errhandler_free(&worldErrorhandler);
        };

    protected:
        MPI_Errhandler teamErrorhandler;
        MPI_Errhandler worldErrorhandler;

};


#endif