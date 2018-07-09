/*
 * RankControl.h
 *
 *  Created on: 2 Jul 2018
 *      Author: Ben Hazelwood
 */

#ifndef RANKCONTROL_H_
#define RANKCONTROL_H_

/* 
    USR1 is used to pause a rank for 1s
    USR2 is used to corrupt the data on next heartbeat
 */
void registerSignalHandler();

// USR1
void pauseThisRankSignalHandler(int signum);

// USR2
void corruptThisRankSignalHandler(int signum);

bool getShouldCorruptData();

void setShouldCorruptData(bool toggle);

#endif