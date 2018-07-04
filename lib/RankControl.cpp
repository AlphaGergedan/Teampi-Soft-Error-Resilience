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
  signal(SIGUSR1, pauseThisRankSignalHandler);
  signal(SIGUSR2, corruptThisRankSignalHandler);
  shouldCorruptData = false;
}

void pauseThisRankSignalHandler( int signum ) { 
  const double sleepLength = 0.1 * 1e6;
  logDebug( "Signal received: sleep for 0.1s");
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