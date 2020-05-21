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
static int worldSize;
static int teamRank;
static int teamSize;
static int numTeams;
static int team;
static int argCount;
static char ***argValues;
static std::function<void(bool)> *loadCheckpointCallback = nullptr;
static std::function<void(void)> *createCheckpointCallback = nullptr;

static MPI_Comm TMPI_COMM_TEAM;
static MPI_Comm TMPI_COMM_INTER_TEAM;
static MPI_Comm TMPI_COMM_WORLD = MPI_COMM_NULL;
static MPI_Comm TMPI_COMM_LIB;
//TODO TMPI_COMM_WORLD should not be the same thing as libComm
static MPI_Errhandler TMPI_ERRHANDLER_COMM_WORLD;
static MPI_Errhandler TMPI_ERRHANDLER_COMM_TEAM;
static TMPI_ErrorHandlingStrategy error_handler = TMPI_KillTeamErrorHandler;
static std::function<void (MPI_Comm)> *rejoin_function = nullptr;
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
    MPI_Comm_create_errhandler(respawn_proc_errh_comm_team, &TMPI_ERRHANDLER_COMM_TEAM);
    MPI_Comm_create_errhandler(respawn_proc_errh_comm_world, &TMPI_ERRHANDLER_COMM_WORLD);
    rejoin_function = new std::function<void (MPI_Comm)>(respawn_proc_recreate_comm_world);
    break;
  case TMPI_KillTeamErrorHandler:
    MPI_Comm_create_errhandler(kill_team_errh_comm_team, &TMPI_ERRHANDLER_COMM_TEAM);
    MPI_Comm_create_errhandler(kill_team_errh_comm_world, &TMPI_ERRHANDLER_COMM_WORLD);
  default:
  MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ARG);
    break;
  }

  if (parent != MPI_COMM_NULL)
  {
    (*rejoin_function)(TMPI_COMM_WORLD);

    PMPI_Comm_size(TMPI_COMM_WORLD, &worldSize);

    PMPI_Comm_rank(TMPI_COMM_WORLD, &worldRank);
    
    PMPI_Comm_rank(TMPI_COMM_TEAM, &teamRank);

    PMPI_Comm_size(TMPI_COMM_TEAM, &teamSize);

    teamSize = worldSize / numTeams;
    team = worldRank / teamSize;

    Timing::initialiseTiming();

    std::cout << "New Proc: Team: " << team << " TeamRank: " << teamRank << " World Rank: " << worldRank << " TeamSize: " << teamSize << std::endl;

    (*loadCheckpointCallback)(true);

    
  }
  else
  {

    /**
   * The application should have no knowledge of the world_size or world_rank
   */
    //TODO only if original spwan
    PMPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    PMPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    teamSize = worldSize / numTeams;

    int color = worldRank / teamSize;
    team = worldRank / teamSize;

    PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_WORLD);

    PMPI_Comm_dup(MPI_COMM_WORLD, &TMPI_COMM_LIB);

    PMPI_Comm_split(MPI_COMM_WORLD, color, worldRank, &TMPI_COMM_TEAM);

    PMPI_Comm_rank(TMPI_COMM_TEAM, &teamRank);

    PMPI_Comm_size(TMPI_COMM_TEAM, &teamSize);

    // Todo: free
    // Todo: failure clean
    PMPI_Comm_split(MPI_COMM_WORLD, teamRank, worldRank, &TMPI_COMM_INTER_TEAM);

    assert(teamSize == (worldSize / numTeams));

    //Error Handling Stuff
    //PMPI_Comm_create_errhandler(respawn_proc_errh_comm_world, &TMPI_ERRHANDLER_COMM_WORLD);
    PMPI_Comm_set_errhandler(TMPI_COMM_WORLD, TMPI_ERRHANDLER_COMM_WORLD);
    PMPI_Comm_set_errhandler(TMPI_COMM_LIB, TMPI_ERRHANDLER_COMM_WORLD);

    //PMPI_Comm_create_errhandler(respawn_proc_errh_comm_team, &TMPI_ERRHANDLER_COMM_TEAM);
    PMPI_Comm_set_errhandler(TMPI_COMM_TEAM, TMPI_ERRHANDLER_COMM_TEAM);

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

  }
  delete(rejoin_function);
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
  return worldSize;
}

void refreshWorldSize()
{
  PMPI_Comm_size(TMPI_COMM_WORLD, &worldSize);
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
  if (worldSize % numTeams != 0)
  {
    std::cerr << "Wrong choice of world size and number of teams!\n";
    std::cerr << "Number of teams: " << numTeams << "\n";
    std::cerr << "Total ranks: " << worldSize << "\n";
  }

  assert(worldSize % numTeams == 0);

  PMPI_Barrier(TMPI_COMM_WORLD);
  double my_time = MPI_Wtime();
  PMPI_Barrier(TMPI_COMM_WORLD);
  double times[worldSize];

  PMPI_Gather(&my_time, 1, MPI_DOUBLE, times, 1, MPI_DOUBLE, MASTER, TMPI_COMM_WORLD);

  PMPI_Barrier(TMPI_COMM_WORLD);

  if (worldRank == MASTER)
  {
    std::cout << "------------TMPI SETTINGS------------\n";
    std::cout << "Number of teams: " << numTeams << "\n";

    std::cout << "Team size: " << teamSize << "\n";
    std::cout << "Total ranks: " << worldSize << "\n\n";

    if (worldRank == MASTER)
    {
      for (int i = 0; i < worldSize; i++)
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
  std::string env(getEnvString("TEAMS"));
  numTeams = env.empty() ? 2 : std::stoi(env);
}

//Kritisch
int mapRankToTeamNumber(int rank)
{
  return rank / getTeamSize();
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

MPI_Errhandler *getWorldErrhandler()
{
  return &TMPI_ERRHANDLER_COMM_WORLD;
}

int getArgCount()
{
  return argCount;
}

char ***getArgValues()
{
  return argValues;
}

std::function<void(void)> *getCreateCheckpointCallback()
{
  assert(createCheckpointCallback != nullptr);
  return createCheckpointCallback;
}
std::function<void(bool)> *getLoadCheckpointCallback()
{
  assert(loadCheckpointCallback != nullptr);
  return loadCheckpointCallback;
}

void setCreateCheckpointCallback(std::function<void(void)> *function)
{
  createCheckpointCallback = function;
}
void setLoadCheckpointCallback(std::function<void(bool)> *function)
{
  loadCheckpointCallback = function;
}

void setErrorHandlingStrategy(TMPI_ErrorHandlingStrategy strategy){
  if(strategy == TMPI_WarmSpareErrorHandler) assert(false);
  error_handler = strategy;
}

