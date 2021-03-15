/*
 * RankOperations.cpp
 *
 *  Created on: 2 Mar 2018
 *      Author: Ben Hazelwood
 */

#include "Rank.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <functional>

#include "RankControl.h"
#include "Logging.h"
#include "Timing.h"


static int worldRank;
static int worldSizeNoSpares;
static int worldSizeSpares;
static int teamRank;
static int teamSize;
static int numTeams;
static int numSpares = 0;
static int team;
static int argCount;
static bool isSpareRank;
static char ***argValues;
static std::function<void(int)> *loadCheckpointCallback = nullptr;
static std::function<void(std::vector<int>)> *createCheckpointCallback = nullptr;

static MPI_Comm TMPI_COMM_TEAM;
static MPI_Comm TMPI_COMM_INTER_TEAM;
static MPI_Comm TMPI_COMM_WORLD = MPI_COMM_NULL;
static MPI_Comm TMPI_COMM_LIB;
static MPI_Errhandler TMPI_ERRHANDLER;
static TMPI_ErrorHandlingStrategy error_handler = TMPI_NoErrorHandler;
static std::function<void (bool)> recreate_function;
static std::function<void ()> wait_function;

int initialiseTMPI(int *argc, char ***argv)
{

  argCount = *argc;
  argValues = argv;
  setEnvironment();

  MPI_Comm parent;
  PMPI_Comm_get_parent(&parent);

  switch (error_handler)
  {
  case TMPI_RespawnProcErrorHandler:
    MPI_Comm_create_errhandler(respawn_proc_errh, &TMPI_ERRHANDLER);
    MPI_Comm_create_errhandler(respawn_proc_errh, &TMPI_ERRHANDLER);
    recreate_function = std::function<void (bool)>(respawn_proc_recreate_world);
    break;
  case TMPI_KillTeamErrorHandler:
    MPI_Comm_create_errhandler(kill_team_errh_comm_world, &TMPI_ERRHANDLER);
    MPI_Comm_create_errhandler(kill_team_errh_comm_world, &TMPI_ERRHANDLER);
    recreate_function = std::function<void(bool)>(kill_team_recreate_world);
    break;
  case TMPI_WarmSpareErrorHandler:
    MPI_Comm_create_errhandler(warm_spare_errh, &TMPI_ERRHANDLER);
    MPI_Comm_create_errhandler(warm_spare_errh, &TMPI_ERRHANDLER);
    recreate_function = std::function<void(bool)>(warm_spare_recreate_world);
    wait_function = std::function<void ()>(warm_spare_wait_function);
    break;
  case TMPI_NoErrorHandler:
    TMPI_ERRHANDLER = MPI_ERRORS_ARE_FATAL;
    TMPI_ERRHANDLER = MPI_ERRORS_ARE_FATAL;
    break;
  default:
    TMPI_ERRHANDLER = MPI_ERRORS_ARE_FATAL;
    TMPI_ERRHANDLER = MPI_ERRORS_ARE_FATAL;
    break;
  }

  if (parent != MPI_COMM_NULL)
  {
    recreate_function(true);

    PMPI_Comm_size(TMPI_COMM_WORLD, &worldSizeNoSpares);

    PMPI_Comm_rank(TMPI_COMM_WORLD, &worldRank);
    
    PMPI_Comm_rank(TMPI_COMM_TEAM, &teamRank);

    PMPI_Comm_size(TMPI_COMM_TEAM, &teamSize);

    teamSize = worldSizeNoSpares / numTeams;
    team = worldRank / teamSize;

    Timing::initialiseTiming();

    std::cout << "New Proc: Team: " << team << " TeamRank: " << teamRank << " World Rank: " << worldRank << " TeamSize: " << teamSize << std::endl;

    registerSignalHandler();
    (*loadCheckpointCallback)(true);

    
  }
  else
  {

    /**
   * The application should have no knowledge of the world_size or world_rank
   */
    //TODO only if original spwan
    PMPI_Comm_size(MPI_COMM_WORLD, &worldSizeNoSpares);
    PMPI_Comm_size(MPI_COMM_WORLD, &worldSizeSpares);
    PMPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    int color;

    if(error_handler == TMPI_WarmSpareErrorHandler){
      PMPI_Comm_size(MPI_COMM_WORLD, &worldSizeSpares);
      worldSizeNoSpares -= numSpares;
      assert(worldSizeNoSpares > 0);
      teamSize = worldSizeNoSpares / numTeams;
      assert(teamSize > 0);
      color = (worldRank >= worldSizeNoSpares) ? numTeams : worldRank / teamSize;
      team = (worldRank >= worldSizeNoSpares) ? numTeams : worldRank / teamSize;
      isSpareRank = (worldRank >= worldSizeNoSpares);

      std::cout << "Is Spare?" << worldRank << ": " << isSpareRank << std::endl;
    } else {
      teamSize = worldSizeNoSpares / numTeams;

      color = worldRank / teamSize;
      team = worldRank / teamSize;

      isSpareRank = false;
    }
    
    PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_WORLD);

    PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_LIB);

    PMPI_Comm_split(MPI_COMM_WORLD, color, worldRank, &TMPI_COMM_TEAM);

    PMPI_Comm_rank(TMPI_COMM_TEAM, &teamRank);

    //PMPI_Comm_size(TMPI_COMM_TEAM, &teamSize);

    // Todo: free
    // Todo: failure clean
    PMPI_Comm_split(MPI_COMM_WORLD, (isSpareRank) ? teamSize : teamRank, worldRank, &TMPI_COMM_INTER_TEAM);

    if(!isSpareRank)assert(teamSize == (worldSizeNoSpares / numTeams));

    //Error Handling Stuff
    //PMPI_Comm_create_errhandler(respawn_proc_errh_comm_world, &TMPI_ERRHANDLER);
    PMPI_Comm_set_errhandler(TMPI_COMM_WORLD, TMPI_ERRHANDLER);
    PMPI_Comm_set_errhandler(TMPI_COMM_LIB, TMPI_ERRHANDLER);

    //PMPI_Comm_create_errhandler(respawn_proc_errh_comm_team, &TMPI_ERRHANDLER);
    PMPI_Comm_set_errhandler(TMPI_COMM_TEAM, TMPI_ERRHANDLER);

    PMPI_Comm_set_errhandler(TMPI_COMM_INTER_TEAM, TMPI_ERRHANDLER);

    registerSignalHandler();
    outputEnvironment();

#ifndef REPLICAS_OUTPUT
    // Disable output for all but master replica (0)
    if (getTeam() > 0)
    {
      //    disableLogging();
    }
#endif

    Timing::initialiseTiming();

    synchroniseRanksGlobally();
    if(isSpareRank) wait_function();

  }

  return MPI_SUCCESS;
}

int getWorldRank()
{
  return worldRank;
}

void refreshWorldRank()
{
  PMPI_Comm_rank(TMPI_COMM_WORLD, &worldRank);
}

int getWorldSize()
{
  return worldSizeNoSpares;
}

void refreshWorldSize()
{
  PMPI_Comm_size(TMPI_COMM_WORLD, &worldSizeSpares);
  worldSizeNoSpares = worldSizeSpares - numSpares;
}

int getTeamRank()
{
  return teamRank;
}

int getTeamSize()
{
  return teamSize;
}

int getTeam()
{
  return team;
}

void setTeam(int newTeam)
{
  team = newTeam;
}

int getNumberOfTeams()
{
  return numTeams;
}

void setNumberOfTeams(int newNumTeams)
{
  numTeams = newNumTeams;
}

void setTeamComm(MPI_Comm comm)
{
  TMPI_COMM_TEAM = comm;
}

MPI_Comm getTeamComm(MPI_Comm comm)
{
  return (comm == MPI_COMM_WORLD) ? TMPI_COMM_TEAM : comm;
}

void setTeamInterComm(MPI_Comm comm)
{
  TMPI_COMM_INTER_TEAM = comm;
}

MPI_Comm getTeamInterComm()
{
  return TMPI_COMM_INTER_TEAM;
}

int freeTeamComm()
{
  return MPI_Comm_free(&TMPI_COMM_TEAM);
}

MPI_Comm getLibComm()
{
  return TMPI_COMM_WORLD;
}

int setLibComm(MPI_Comm comm)
{
  TMPI_COMM_WORLD = comm;
  return 0;
}

int freeLibComm()
{
  return MPI_Comm_free(&TMPI_COMM_WORLD);
}

MPI_Comm getWorldComm()
{
  return TMPI_COMM_WORLD;
}

void setWorldComm(MPI_Comm newComm)
{
  TMPI_COMM_WORLD = newComm;
}
std::string getEnvString(std::string const &key)
{
  char const *val = std::getenv(key.c_str());
  return val == nullptr ? std::string() : std::string(val);
}

void outputEnvironment()
{
  if (worldSizeNoSpares % numTeams != 0)
  {
    std::cerr << "Wrong choice of world size and number of teams!\n";
    std::cerr << "Number of teams: " << numTeams << "\n";
    std::cerr << "Total ranks no spare ranks: " << worldSizeNoSpares << "\n";
  }

  assert(worldSizeNoSpares % numTeams == 0);

  PMPI_Barrier(TMPI_COMM_WORLD);
  double my_time = MPI_Wtime();
  PMPI_Barrier(TMPI_COMM_WORLD);
  double times[worldSizeSpares];

  PMPI_Gather(&my_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, MASTER, TMPI_COMM_WORLD);

  PMPI_Barrier(TMPI_COMM_WORLD);

  if (worldRank == MASTER)
  {
    std::cout << "------------TMPI SETTINGS------------\n";
    std::cout << "Number of teams: " << numTeams << "\n";

    std::cout << "Team size: " << teamSize << "\n";
    std::cout << "Total ranks: " << worldSizeSpares << "\n\n";

    if (worldRank == MASTER)
    {
      for (int i = 0; i < worldSizeSpares; i++)
      {
        std::cout << "Tshift(" << i << "->" << mapWorldToTeamRank(i) << "->" << mapTeamToWorldRank(mapWorldToTeamRank(i), mapRankToTeamNumber(i)) << ") = " << times[i] - times[0] << "\n";
      }
    }
    std::cout << "---------------------------------------\n\n";
  }

  PMPI_Barrier(TMPI_COMM_WORLD);
}

void setEnvironment()
{
  std::string teamsStr(getEnvString("TEAMS"));
  std::string sparesStr(getEnvString("SPARES"));
  numTeams = teamsStr.empty() ? 2 : std::stoi(teamsStr);
  numSpares = sparesStr.empty() ? 0 : std::stoi(sparesStr); 

}

//Kritisch
int mapRankToTeamNumber(int rank)
{
  int ret = rank / getTeamSize();
  if(ret >= numTeams) return numTeams;
  return ret;
}

//Kritisch
int mapWorldToTeamRank(int rank)
{
  if (rank == MPI_ANY_SOURCE)
  {
    return MPI_ANY_SOURCE;
  }
  else
  {
    return rank % getTeamSize();
  }
}

//Kritisch
int mapTeamToWorldRank(int rank, int r)
{
  if (rank == MPI_ANY_SOURCE)
  {
    return MPI_ANY_SOURCE;
  }

  return rank + r * getTeamSize();
}

void remapStatus(MPI_Status *status)
{
  if ((status != MPI_STATUS_IGNORE) && (status != MPI_STATUSES_IGNORE))
  {
    status->MPI_SOURCE = mapWorldToTeamRank(status->MPI_SOURCE);
    logInfo(
        "remap status source " << status->MPI_SOURCE << " to " << mapWorldToTeamRank(status->MPI_SOURCE));
  }
}

int synchroniseRanksInTeam()
{
   int err = PMPI_Barrier(getTeamComm(MPI_COMM_WORLD));
   //std::cout << "Barrier complete: Team: " << getTeam() << " Rank: " << getTeamRank() << std::endl;
   return err;
}

int synchroniseRanksGlobally()
{
  return PMPI_Barrier(getLibComm());
}

MPI_Errhandler *getErrhandler()
{
  return &TMPI_ERRHANDLER;
}

int getArgCount()
{
  return argCount;
}

char ***getArgValues()
{
  return argValues;
}

std::function<void(std::vector<int>)> *getCreateCheckpointCallback()
{
  assert(createCheckpointCallback != nullptr);
  return createCheckpointCallback;
}
std::function<void(int)> *getLoadCheckpointCallback()
{
  assert(loadCheckpointCallback != nullptr);
  return loadCheckpointCallback;
}

void setCreateCheckpointCallback(std::function<void(std::vector<int>)> *function)
{
  createCheckpointCallback = function;
}
void setLoadCheckpointCallback(std::function<void(int)> *function)
{
  loadCheckpointCallback = function;
}

void setErrorHandlingStrategy(TMPI_ErrorHandlingStrategy strategy){
  error_handler = strategy;
}

int getNumberOfSpares(){
  return numSpares;
}

void setNumberOfSpares(int spares){
  numSpares = spares;
}

bool isSpare(){
  return isSpareRank;
}

void setSpare(bool status){
  isSpareRank = status;
}

void setTeamRank(int rank){
  teamRank = rank;
}

std::function<void(bool)>* getRecreateWorldFunction(){
  if(error_handler == TMPI_NoErrorHandler) return nullptr;
  return &recreate_function;
}
