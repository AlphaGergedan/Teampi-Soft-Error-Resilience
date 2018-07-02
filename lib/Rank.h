/*
 * RankOperations.h
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
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
   #error "what is happening here?"
#endif

int initialiseTMPI();

int getWorldRank();

int getWorldSize();

int getTeamRank();

int getTeamSize();

int getNumberOfTeams();

int getTeam();

MPI_Comm getTeamComm();

int freeTeamComm();

MPI_Comm getLibComm();
int freeLibComm();

std::string getEnvString(std::string const& key);

void setEnvironment();

void outputEnvironment();

void outputTiming();

void pauseThisRankSignalHandler(int signum);

void corruptThisRankSignalHandler(int signum);

bool getShouldCorruptData();
void setShouldCorruptData(bool toggle);

int mapRankToTeamNumber(int rank);

int mapWorldToTeamRank(int rank);

int mapTeamToWorldRank(int rank, int r);

void remapStatus(MPI_Status *status);

int synchroniseRanksInTeam();
int synchroniseRanksGlobally();



#endif /* RANK_H_ */
