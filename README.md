# tMPI: A team based wrapper for MPI #

### What is this repository for? ###

This wrapper adds functionality to MPI enabling replication and team based execution.

The library re-defines the weak symbols declared by MPI. This means the replication is transparent to the application.

Unlike other similar libraries, the replicas are free to proceed completely asynchronously from other replicas. They communicate progress and error information via a "heartbeat". A heartbeat is inserted into user via an MPI_Sendrecv(...,MPI_COMM_SELF) call. At each heartbeat, the replicas mark this point on a timeline and compare there progress against the other replicas. If a buffer is provided to the heartbeat then the hashcode is also compared with the other replicas. 

### How do I get set up? ###
* Run make in the lib directory 
* Optional: rum make in the applications/tests directory
* Link with -ltmpi -L<path to tmpi> 
* Add <path to tmpi> to LD_LIBRARY_PATH

### Who do I talk to? ###
Ben Hazelwood (benjamin.hazelwood@durham.ac.uk)