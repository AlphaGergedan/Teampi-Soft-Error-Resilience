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

static bool shouldCorruptData;

void registerSignalHandler() {
  #ifdef PAUSE_RANK
  signal(SIGUSR1, pauseThisRankSignalHandler);
  #endif
  #ifdef CORRUPT_RANK
  signal(SIGUSR2, corruptThisRankSignalHandler);
  #endif
  shouldCorruptData = false;
}

void pauseThisRankSignalHandler( int signum ) { 
  const double sleepLength = 0.1 * 1e6;
  logInfo( "Signal received: sleep for 0.1s");
  usleep(sleepLength);
}

void corruptThisRankSignalHandler( int signum ) {
  logInfo("Signal received: corrupt this rank");
  shouldCorruptData = true;
}

bool getShouldCorruptData() {
  return shouldCorruptData;
}

void setShouldCorruptData(bool toggle) {
  shouldCorruptData = toggle;
}