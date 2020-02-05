
/*
 * CommStats.cpp
 *
 *  Created on: 4 Feb 2020
 *      Author: Philipp Samfass
 */

#include "CommStats.h"
#include "Logging.h"
#include "Rank.h"
#include "RankControl.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <stddef.h>
#include <bitset>
#include <unistd.h>
#include <list>
#include <vector>
#include <atomic>

struct CommunicationStats {
  std::atomic<size_t> sentBytes; 
  std::atomic<size_t> receivedBytes;
} stats; 

size_t CommunicationStatistics::computeCommunicationVolume(MPI_Datatype datatype, int count) {
  int size;
  MPI_Type_size(datatype, &size);
  return static_cast<size_t>(count)*size;
}

void CommunicationStatistics::trackSend(MPI_Datatype datatype, int count) {
  stats.sentBytes += computeCommunicationVolume(datatype, count); 
}

void CommunicationStatistics::trackReceive(MPI_Datatype datatype, int count) {
  stats.receivedBytes += computeCommunicationVolume(datatype, count);
}

void CommunicationStatistics::outputCommunicationStatistics() {  
  std::cout.flush(); \
  std::cout << "[TMPI]    [rank " << getTeamRank() << "/" << getWorldRank() << "]  bytes sent: "<< stats.sentBytes.load()
                                                                               <<" bytes received: " << stats.receivedBytes.load()<< std::endl; \
 
  std::cout.flush(); 

  std::string filenamePrefix = getEnvString("TMPI_STATS_FILE");
  std::string outputPathPrefix = getEnvString("TMPI_STATS_OUTPUT_PATH");

  if (!filenamePrefix.empty()) {
    // Write Generic Sync points to files
    char sep = ',';
    std::ostringstream filename;
    std::string outputFolder(outputPathPrefix.empty() ? "tmpi-statistics" : outputPathPrefix);
    filename << outputFolder << "/"
        << filenamePrefix << "-"
        << getWorldRank() << "-"
        << getTeamRank() << "-"
        << getTeam()
        << ".csv";
    std::ofstream f;
    f.open(filename.str().c_str());

    logInfo("Writing statistics to " << filename);

    f << "sent received"<<"\n";
    f << stats.sentBytes.load() << " "<< stats.receivedBytes.load();

    f.close();
  } 
}
