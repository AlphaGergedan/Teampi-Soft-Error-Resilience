#ifndef WARMSPARESTRATEGY_H_
#define WARMSPARESTRATEGY_H_

#include <mpi.h>

    void warm_spare_errh(MPI_Comm *pcomm, int *perr, ...);
    void warm_spare_rejoin_function(bool  isSpare);
    void warm_spare_wait_function();
   
#endif