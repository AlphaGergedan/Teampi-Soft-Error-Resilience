/*
 * RankOperations.h
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#ifndef RANKOPERATIONS_H_
#define RANKOPERATIONS_H_

#include <mpi.h>
#include <string>


int getWorldRank();

int getWorldSize();

int getTeamRank();

int getTeamSize();

int getNumberOfReplicas();

MPI_Comm getReplicaCommunicator();

int freeReplicaCommunicator();

MPI_Comm getTMPICommunicator();
int freeTMPICommunicator();

std::string getEnvString(std::string const& key);

void read_config();

void print_config();

void output_timing();

void pauseThisRankSignalHandler( int signum );

/**
 * Sets the global variables for an MPI process
 * @return MPI_SUCCESS
 */
int init_rank();

/**
 * @param rank of process (default is this rank)
 * @return which replica rank is [0..R_FACTOR]
 */
int get_R_number(int rank=-1);

/**
 * @param world_rank
 * @return world_rank mapped to a team_rank
 */
int map_world_to_team(int world_rank);

/**
 *
 * @param rank of a replica
 * @param r_num specifying which replica rank is
 * @return rank mapped to a world/global rank
 */
int map_team_to_world(int rank, int r_num=-1);

/**
 * Modify the source of the status to within team_size
 * @param status to modify
 */
void remap_status(MPI_Status *status);


#endif /* RANKOPERATIONS_H_ */
