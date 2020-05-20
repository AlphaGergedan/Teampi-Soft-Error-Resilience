#ifndef ERRORHANDLINGSTRATEGIES_H_
#define ERRORHANDLINGSTRATEGIES_H_
enum TMPI_ErrorHandlingStrategy{
    TMPI_KillTeamErrorHandler = 0,
    TMPI_RespawnProcErrorHandler = 1,
    TMPI_WarmSpareErrorHandler = 2
};
#endif