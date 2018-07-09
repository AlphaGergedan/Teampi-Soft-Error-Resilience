/*
 * RankOperations.cpp
 *
 *  Created on: 2 Jul 2018
 *      Author: Ben Hazelwood
 */


#include <csignal>
#include <unistd.h>

#include "RankControl.h"
#include "Logging.h"

static bool shouldSleepRank;
static bool shouldCorruptData;


void registerSignalHandler() {
  signal(SIGUSR1, pauseThisRankSignalHandler);
  signal(SIGUSR2, corruptThisRankSignalHandler);
  shouldCorruptData = false;
}

void pauseThisRankSignalHandler( int signum ) { 
  shouldSleepRank = true;
}

void corruptThisRankSignalHandler( int signum ) {
  logInfo("Signal received: corrupt this rank on next heartbeart");
  shouldCorruptData = true;
}

bool getShouldSleepRank() {
  return shouldSleepRank;
}

void setShouldSleepRank(bool toggle) {
  shouldSleepRank = toggle;
}

bool getShouldCorruptData() {
  return shouldCorruptData;
}

void setShouldCorruptData(bool toggle) {
  shouldCorruptData = toggle;
}

