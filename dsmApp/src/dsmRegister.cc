/*
FILENAME...	dsmRegister.cc
USAGE...	Register DSM motor device driver shell commands.

*/

/*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

This software was developed under a United States Government license
described on the COPYRIGHT_UniversityOfChicago file included as part
of this distribution.
**********************************************************************/

#include <iocsh.h>
#include "dsmRegister.h"
#include "epicsExport.h"

extern "C"
{

// DSM Setup arguments
static const iocshArg setupArg0 = {"Max. controller count", iocshArgInt};
static const iocshArg setupArg1 = {"Polling rate", iocshArgInt};
// DSM Config arguments
static const iocshArg configArg0 = {"Card being configured", iocshArgInt};
static const iocshArg configArg1 = {"asyn port name", iocshArgString};

static const iocshArg * const MD90SetupArgs[2]  = {&setupArg0, &setupArg1};
static const iocshArg * const MD90ConfigArgs[2] = {&configArg0, &configArg1};

static const iocshFuncDef setupMD90  = {"MD90Setup",  2, MD90SetupArgs};
static const iocshFuncDef configMD90 = {"MD90Config", 2, MD90ConfigArgs};

static void setupMD90CallFunc(const iocshArgBuf *args)
{
    MD90Setup(args[0].ival, args[1].ival);
}
static void configMD90CallFunc(const iocshArgBuf *args)
{
    MD90Config(args[0].ival, args[1].sval);
}

static void dsmRegister(void)
{
    iocshRegister(&setupMD90, setupMD90CallFunc);
    iocshRegister(&configMD90, configMD90CallFunc);
}

epicsExportRegistrar(dsmRegister);

} // extern "C"
