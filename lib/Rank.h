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

#define MASTER 0

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

int mapRankToTeamNumber(int rank);

int mapWorldToTeamRank(int rank);

int mapTeamToWorldRank(int rank, int r);

void remapStatus(MPI_Status *status);


#endif /* RANK_H_ */
