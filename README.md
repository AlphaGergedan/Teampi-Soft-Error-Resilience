# teaMPI with Soft Error Resilience #

See the original repository here: https://gitlab.lrz.de/hpcsoftware/teaMPI

This repository tries to integrate soft error resilience to the teaMPI library. <br>
(updating on branch ulfm\_failure\_tolerance)

## Soft Error Resilience ##

Increased error rates in high performance computing due to multiple cores and
memories may lead to silent data corruptions. In such a case, we are unable to
detect the error without observing the application's results.

Keep in mind:
- don't break teaMPI's asynchronization
- don't break warmSpares. So we don't break fault tolerance

### First Method: Comparing Hashes using Heartbeats ###

TODO

### Second Method: ###

TODO

## teaMPI library ###

This wrapper adds functionality to MPI enabling replication and team based execution using the PMPI features of MPI.

The library re-defines the weak symbols declared by MPI. This means the replication is transparent to the application.

Replication is achieved by splitting `MPI_COMM_WORLD` into a fixed number of "team" communicators.
Each team then sends messages completely independent of other teams.

Unlike other similar libraries, the replicas are free to proceed completely asynchronously from other replicas.
They communicate progress and error information via a "heartbeat". A heartbeat is inserted into user code via
an `MPI_Sendrecv(...,MPI_COMM_SELF)` call. At each heartbeat, the replicas mark this point on a timeline and
compare there progress against the other replicas. If a buffer is provided to the heartbeat then the hashcode
is also compared with the other replicas.

### How do I get set up? ##
To build the library:
1. Set compiler in the makefile to the path of your ULFM compiler
2. Run `make` in the lib directory
3. set the number of teams with the `TEAMS` environment variable (default: 2)

To use some example provided miniapps:
1. run `make` in the applications folder
2. run each application in the bin folder with the required command line parameters (documented in each application folder)

To use with an existing application:
1. Link with `-ltmpi -L"path to teaMPI"`
2. Add "path to teaMPI" to `LD_LIBRARY_PATH`

To try out fault tolerance functionality:
1. Fault tolerance features used in applications/MinimalTest
2. Change ErrorHandler to desired mode
3. For Warm-Spares make sure to launch with enough processes and set number of spares with `SPARES` environment var
4. For Cold-Spares make sure that you have enough slots available. Easiest way to achieve this is by using the --oversubscribe option.
5. When implementing own fault tolerant code be sure to call the hearbeat `using MPI_Allreduce` as demonstrated in the example.
### Example Heartbeat Usage ###
This application models many scientific applications. Per loop, the two `MPI_Sendrecv` calls act as heartbeats.
The first starts the timer for this rank and the second stops it. Additionally the second heartbeat passes
the data buffer for comparison with other teams. Only a hash of the data is sent.

At the end of the application, the heartbeat times will be written to CSV files.



```C++
double data[SIZE];
for (int t = 0; t < NUM_TRIALS; t++)
{
    MPI_Barrier(MPI_COMM_WORLD);

    // Start Heartbeat
    MPI_Sendrecv(MPI_IN_PLACE, 0, MPI_BYTE, MPI_PROC_NULL, 1, MPI_IN_PLACE, 0,
        MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);

    for (int i = 0; i < NUM_COMPUTATIONS; i++) {
        // Arbitrary computation on data
    }

    // End Heartbeat and compare data
    MPI_Sendrecv(data, SIZE, MPI_DOUBLE, MPI_PROC_NULL, -1, MPI_IN_PLACE, 0,
        MPI_BYTE, MPI_PROC_NULL, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);

    MPI_Barrier(MPI_COMM_WORLD);
}
```

### What if I want to communicate between teams myself? ###
To get access to the original MPI function, prefix it with a P. For example, MPI\_Send becomes PMPI\_Send.

If you wish to map between original rank numbers, know how many teams there are, perform a global barrier, etc. take a look at the Rank.h file.
It has access to all the data internal to teaMPI. To use the functions declared in it, simply `#include "Rank.h"` and add the path to `teampi/lib` to the compilation `-I` flags.


### Who do I talk to? ###
Ben Hazelwood (benjamin.hazelwood@durham.ac.uk)
Tobias Weinzierl (tobias.weinzierl@durham.ac.uk)
Philipp Samfass (samfass@in.tum.de)
