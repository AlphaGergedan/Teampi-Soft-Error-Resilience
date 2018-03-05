/*
 * Logging.h
 *
 *  Created on: 2 Mar 2018
 *      Author: ben
 */

#ifndef LOGGING_H_
#define LOGGING_H_

#ifdef LOGINFO
#define logInfo(messageStream) \
   { \
      std::cout << "[TMPI]    [rank " << team_rank << "/" << world_rank << "]    " << messageStream << std::endl; \
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
      std::cout << "[TMPI]    [rank " << team_rank << "/" << world_rank << "]    " << messageStream << std::endl; \
      std::cout.flush(); \
   }

#endif /* LOGGING_H_ */
