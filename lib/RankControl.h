/*
 * RankControl.h
 *
 *  Created on: 2 Jul 2018
 *      Author: Ben Hazelwood
 */

#ifndef RANKCONTROL_H_
#define RANKCONTROL_H_

void registerSignalHandler();

void pauseThisRankSignalHandler(int signum);

void corruptThisRankSignalHandler(int signum);

bool getShouldCorruptData();

void setShouldCorruptData(bool toggle);

#endif