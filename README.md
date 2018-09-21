# tMPI: A team based PMPI wrapper for MPI #

### What is this repository for? ###

This wrapper adds functionality to MPI enabling replication and team based execution using the PMPI features of MPI. 

The library re-defines the weak symbols declared by MPI. This means the replication is transparent to the application.

Replication is achieved by splitting MPI_COMM_WORLD into a fixed number of "team" communicators. Each team then sends messages completely independent of other teams. 

Unlike other similar libraries, the replicas are free to proceed completely asynchronously from other replicas. They communicate progress and error information via a "heartbeat". A heartbeat is inserted into user code via an MPI_Sendrecv(...,MPI_COMM_SELF) call. At each heartbeat, the replicas mark this point on a timeline and compare there progress against the other replicas. If a buffer is provided to the heartbeat then the hashcode is also compared with the other replicas. 

### How do I get set up? ##
To build the library:
* Run make in the lib directory
* set the number of teams with the "TEAMS" environment variable (default: 2)

To use some example provided miniapps:
* run make in the applications folder
* run each application in the bin folder with the required command line parameters (documented in each application folder)

To use with an existing application:
* Link with -ltmpi -L<path to tmpi> 
* Add <path to tmpi> to LD_LIBRARY_PATH

### Who do I talk to? ###
Ben Hazelwood (benjamin.hazelwood@durham.ac.uk)

