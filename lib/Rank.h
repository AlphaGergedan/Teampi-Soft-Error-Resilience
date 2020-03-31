/*
 * RankOperations.h
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood, Philipp Samfass
 */

#ifndef RANK_H_
#define RANK_H_

#include <mpi.h>
#include <string>
#include <stdint.h>
#include <limits.h>

#define MASTER 0

// From https://stackoverflow.com/questions/40807833/sending-size-t-type-data-with-mpi
#if SIZE_MAX == UCHAR_MAX
   #define TMPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define TMPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define TMPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define TMPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define TMPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "Cannot decipher SIZE_MAX"
#endif



/* Split ranks into teams */
int initialiseTMPI();

int getWorldRank();

int getWorldSize();

/* Get the rank as seen by the application */
int getTeamRank();

/* Get the number of ranks as seen by the application */
int getTeamSize();

/* Also the number of replicas */
int getNumberOfTeams();

/* Return which team this rank belongs to */
int getTeam();

/* The communicator used by this team */
MPI_Comm getTeamComm(MPI_Comm comm);
int freeTeamComm();

MPI_Comm getTeamInterComm();

/* The duplicate MPI_COMM_WORLD used by the library*/
MPI_Comm getLibComm();
int setLibComm(MPI_Comm comm);
int freeLibComm();

/* Get the value of an environment variable (empty string if undefined) */
std::string getEnvString(std::string const& key);

/* Get the number of teams from environment  */
void setEnvironment();

/* Output team sizes and any timing inaccuracies between ranks */
void outputEnvironment();

/* Output the timing differences between replicas */
void outputTiming();

/* Decide whether data should be manually corrupted upon next heartbeat */
bool getShouldCorruptData();
void setShouldCorruptData(bool toggle);


int mapRankToTeamNumber(int rank);

int mapWorldToTeamRank(int rank);

int mapTeamToWorldRank(int rank, int r);

/* Alters the MPI_SOURCE member of MPI_Status to 0 <= r < team size */
void remapStatus(MPI_Status *status);

/* Barrier on team communicator */
int synchroniseRanksInTeam();

/* Barrier on all ranks (not called by application) */
int synchroniseRanksGlobally();



#endif /* RANK_H_ */
