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
  const int startValue = 1e4;
  #if SLEEP_MULT != 0
  const int multiplier = SLEEP_MULT;
  #else
  const int multiplier = ((double) rand() / (RAND_MAX)) + 1;
  #endif 

  static int sleepLength = startValue;
  logInfo( "Signal received: sleep for " << (double)sleepLength / 1e6 << "s");
  usleep(sleepLength);
  sleepLength *= multiplier;
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