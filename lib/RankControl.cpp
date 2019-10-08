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
#include "Timing.h"

static bool shouldCorruptData;
static double sleepIncrement = 1 * 1e6;
static double currentSleepLength = 1 * 1e6;

void registerSignalHandler() {
  signal(SIGUSR1, pauseThisRankSignalHandler);
  signal(SIGUSR2, corruptThisRankSignalHandler);
  shouldCorruptData = false;
  
}

void pauseThisRankSignalHandler( int signum ) { 
  Timing::sleepRankRaised();
  //const double sleepLength = 1.0 * 1e6;
  //const double sleepLength = 1.0 * 1e4;
  printf("sleeping for %f\n", currentSleepLength);
  logDebug( "Signal received: sleep for 1s");
  usleep(currentSleepLength);
  currentSleepLength += sleepIncrement;
}

void corruptThisRankSignalHandler( int signum ) {
  logInfo("Signal received: corrupt this rank on next heartbeart");
  shouldCorruptData = true;
}

bool getShouldCorruptData() {
  return shouldCorruptData;
}

void setShouldCorruptData(bool toggle) {
  shouldCorruptData = toggle;
}

