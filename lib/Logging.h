/*
 * Logging.h
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <iostream>
#include "Rank.h"

#ifdef LOGINFO
#define logInfo(messageStream) \
   { \
      std::cout.flush(); \
      std::cout << "[TMPI]    [rank " << getTeamRank() << "/" << getWorldRank() << "]    " << messageStream << std::endl; \
      std::cout.flush(); \
   }
#else
#define logInfo(messageStream) \
   { \
   }
#endif

/**
 * Used as a quick as easy debugging tool, always output.
 */
#define logDebug(messageStream) \
   { \
      std::cout.flush(); \
      std::cout << "[TMPI]    [rank " << getTeamRank() << "/" << getWorldRank() << "]    " << messageStream << std::endl; \
      std::cout.flush(); \
   }

// Disables output (cout, cerr) for this rank 
inline void disableLogging() {
  std::cout.setstate(std::ios_base::failbit);
  std::cerr.setstate(std::ios_base::failbit);
}


#endif /* LOGGING_H_ */
