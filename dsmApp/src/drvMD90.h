/* File: drvMD90.h             */


/* Device Driver Support definitions for motor */
/*
 *      Original Author: Mark Rivers
 *      Current Author: Mark Rivers
 *      Date: 2/24/3002
 *
 * Modification Log:
 * -----------------
 * .01  02/24/2002  mlr  initialized from drvPM304.h
 */

#ifndef	INCdrvMD90h
#define	INCdrvMD90h 1

#include "motordrvCom.h"
#include "asynDriver.h"

/* MD90 default profile. */

#define MD90_NUM_CARDS           4
#define MD90_NUM_CHANNELS        4

#define OUTPUT_TERMINATOR "\r"

struct MD90controller
{
    asynUser *pasynUser;   /* asynUser structure */
    char port[80];   /* asyn port name */
};

#endif	/* INCdrvMD90h */
