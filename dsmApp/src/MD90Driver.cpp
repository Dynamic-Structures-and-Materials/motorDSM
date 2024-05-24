/*
FILENAME... MD90Driver.cpp
USAGE...    Motor driver support for the DSM MD-90 controller.

Mark Rivers
March 1, 2012

*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <iocsh.h>
#include <epicsThread.h>

#include <asynOctetSyncIO.h>

#include <epicsExport.h>
#include "MD90Driver.h"

#define NINT(f) (int)((f)>0 ? (f)+0.5 : (f)-0.5)

/** Creates a new MD90Controller object.
  * \param[in] portName          The name of the asyn port that will be created for this driver
  * \param[in] MD90PortName     The name of the drvAsynSerialPort that was created previously to connect to the MD90 controller 
  * \param[in] numAxes           The number of axes that this controller supports 
  * \param[in] movingPollPeriod  The time between polls when any axis is moving 
  * \param[in] idlePollPeriod    The time between polls when no axis is moving 
  */
MD90Controller::MD90Controller(const char *portName, const char *MD90PortName, int numAxes, 
                                 double movingPollPeriod, double idlePollPeriod)
  :  asynMotorController(portName, numAxes, NUM_MD90_PARAMS, 
                         0, // No additional interfaces beyond those in base class
                         0, // No additional callback interfaces beyond those in base class
                         ASYN_CANBLOCK | ASYN_MULTIDEVICE, 
                         1, // autoconnect
                         0, 0)  // Default priority and stack size
{
  int axis;
  asynStatus status;
  MD90Axis *pAxis;
  static const char *functionName = "MD90Controller::MD90Controller";

  /* Connect to MD90 controller */
  status = pasynOctetSyncIO->connect(MD90PortName, 0, &pasynUserController_, NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
      "%s: cannot connect to MD-90 controller\n",
      functionName);
  }
  for (axis=0; axis<numAxes; axis++) {
    pAxis = new MD90Axis(this, axis);
  }

  startPoller(movingPollPeriod, idlePollPeriod, 2);
}


/** Creates a new MD90Controller object.
  * Configuration command, called directly or from iocsh
  * \param[in] portName          The name of the asyn port that will be created for this driver
  * \param[in] MD90PortName       The name of the drvAsynIPPPort that was created previously to connect to the MD90 controller 
  * \param[in] numAxes           The number of axes that this controller supports 
  * \param[in] movingPollPeriod  The time in ms between polls when any axis is moving
  * \param[in] idlePollPeriod    The time in ms between polls when no axis is moving 
  */
extern "C" int MD90CreateController(const char *portName, const char *MD90PortName, int numAxes, 
                                   int movingPollPeriod, int idlePollPeriod)
{
  MD90Controller *pMD90Controller
    = new MD90Controller(portName, MD90PortName, numAxes, movingPollPeriod/1000., idlePollPeriod/1000.);
  pMD90Controller = NULL;
  return(asynSuccess);
}

/** Reports on status of the driver
  * \param[in] fp The file pointer on which report information will be written
  * \param[in] level The level of report detail desired
  *
  * If details > 0 then information is printed about each axis.
  * After printing controller-specific information it calls asynMotorController::report()
  */
void MD90Controller::report(FILE *fp, int level)
{
  fprintf(fp, "MD-90 motor driver %s, numAxes=%d, moving poll period=%f, idle poll period=%f\n", 
    this->portName, numAxes_, movingPollPeriod_, idlePollPeriod_);

  // Call the base class method
  asynMotorController::report(fp, level);
}

/** Returns a pointer to an MD90Axis object.
  * Returns NULL if the axis number encoded in pasynUser is invalid.
  * \param[in] pasynUser asynUser structure that encodes the axis index number. */
MD90Axis* MD90Controller::getAxis(asynUser *pasynUser)
{
  return static_cast<MD90Axis*>(asynMotorController::getAxis(pasynUser));
}

/** Returns a pointer to an MD90Axis object.
  * Returns NULL if the axis number encoded in pasynUser is invalid.
  * \param[in] axisNo Axis index number. */
MD90Axis* MD90Controller::getAxis(int axisNo)
{
  return static_cast<MD90Axis*>(asynMotorController::getAxis(axisNo));
}


// These are the MD90Axis methods

/** Creates a new MD90Axis object.
  * \param[in] pC Pointer to the MD90Controller to which this axis belongs. 
  * \param[in] axisNo Index number of this axis, range 0 to pC->numAxes_-1.
  * 
  * Initializes register numbers, etc.
  */
MD90Axis::MD90Axis(MD90Controller *pC, int axisNo)
  : asynMotorAxis(pC, axisNo),
    pC_(pC)
{  
}

/** Reports on status of the axis
  * \param[in] fp The file pointer on which report information will be written
  * \param[in] level The level of report detail desired
  *
  * After printing device-specific information calls asynMotorAxis::report()
  */
void MD90Axis::report(FILE *fp, int level)
{
  if (level > 0) {
    fprintf(fp, "  axis %d\n",
            axisNo_);
  }

  // Call the base class method
  asynMotorAxis::report(fp, level);
}

asynStatus MD90Axis::sendAccelAndVelocity(double acceleration, double velocity) 
{
  asynStatus status;
  int ival;
  // static const char *functionName = "MD90::sendAccelAndVelocity";

  // Send the velocity
  ival = NINT(fabs(115200./velocity));
  if (ival < 2) ival=2;
  if (ival > 255) ival = 255;
  sprintf(pC_->outString_, "#%02dV=%d", axisNo_, ival);
  status = pC_->writeReadController();

  // Send the acceleration
  // acceleration is in steps/sec/sec
  // MD-90 is programmed with Ramp Index (R) where:
  //   dval (steps/sec/sec) = 720,000/(256-R) */
  //   or R=256-(720,000/dval) */
  ival = NINT(256-(720000./acceleration));
  if (ival < 1) ival=1;
  if (ival > 255) ival=255;
  sprintf(pC_->outString_, "#%02dR=%d", axisNo_, ival);
  status = pC_->writeReadController();
  return status;
}


asynStatus MD90Axis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status;
  // static const char *functionName = "MD90Axis::move";

  status = sendAccelAndVelocity(acceleration, maxVelocity);
  
  // Position specified in nanometers
  if (relative) {
    sprintf(pC_->outString_, "CRM %d", NINT(position));
  } else {
    sprintf(pC_->outString_, "CLM %d", NINT(position));
  }
  status = pC_->writeReadController();
  return status;
}

asynStatus MD90Axis::home(double minVelocity, double maxVelocity, double acceleration, int forwards)
{
  asynStatus status;
  // static const char *functionName = "MD90Axis::home";

  status = sendAccelAndVelocity(acceleration, maxVelocity);

  sprintf(pC_->outString_, "HOM");
  status = pC_->writeReadController();
  return status;
}

asynStatus MD90Axis::moveVelocity(double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status;
  static const char *functionName = "MD90Axis::moveVelocity";

  asynPrint(pasynUser_, ASYN_TRACE_FLOW,
    "%s: minVelocity=%f, maxVelocity=%f, acceleration=%f\n",
    functionName, minVelocity, maxVelocity, acceleration);
    
  status = sendAccelAndVelocity(acceleration, maxVelocity);

  /* MD-90 does not have jog command. Move 1 million steps */
  if (maxVelocity > 0.) {
    /* This is a positive move in MD90 coordinates */
    sprintf(pC_->outString_, "#%02dI+1000000", axisNo_);
  } else {
      /* This is a negative move in MD90 coordinates */
      sprintf(pC_->outString_, "#%02dI-1000000", axisNo_);
  }
  status = pC_->writeReadController();
  return status;
}

asynStatus MD90Axis::stop(double acceleration )
{
  asynStatus status;
  //static const char *functionName = "MD90Axis::stop";

  sprintf(pC_->outString_, "STP");
  status = pC_->writeReadController();
  return status;
}

asynStatus MD90Axis::setPosition(double position)
{
  asynStatus status;
  //static const char *functionName = "MD90Axis::setPosition";

  sprintf(pC_->outString_, "#%02dP=%+d", axisNo_, NINT(position));
  status = pC_->writeReadController();
  return status;
}

asynStatus MD90Axis::setClosedLoop(bool closedLoop)
{
  asynStatus status;
  //static const char *functionName = "MD90Axis::setClosedLoop";

  sprintf(pC_->outString_, "#%02dW=%d", axisNo_, closedLoop ? 1:0);
  status = pC_->writeReadController();
  return status;
}

/** Polls the axis.
  * This function reads the motor position, the limit status, the home status, the moving status, 
  * and the drive power-on status. 
  * It calls setIntegerParam() and setDoubleParam() for each item that it polls,
  * and then calls callParamCallbacks() at the end.
  * \param[out] moving A flag that is set indicating that the axis is moving (true) or done (false). */
asynStatus MD90Axis::poll(bool *moving)
{ 
  int replyStatus;
  char replyString[256];
  int replyValue;
  int done;
  int driveOn;
  int home;
  double position;
  asynStatus comStatus;

  // TODO:  Will need to add some more error handling for the motor return codes.

  // Read the current motor position
  sprintf(pC_->outString_, "GEC");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Current position in encoder counts: 1000"
  sscanf (pC_->inString_, "%d: %[^:]: %lf", &replyStatus, replyString, &position);
  setDoubleParam(pC_->motorPosition_, position);

  // Read the moving status of this motor
  sprintf(pC_->outString_, "STA");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Current status value: 0"
  sscanf (pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  done = (replyValue == '2') ? 0:1;
  setIntegerParam(pC_->motorStatusDone_, done);
  *moving = done ? false:true;
  switch(replyValue) {
    case 0:  // Idle
        break;
    case 1:  // Open loop move complete
        break;
    case 2:  // Move in progress
        break;
    case 3:  // Move stopped
        break;
    case 4:  // Homing error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        break;
    case 5:  // Stance error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        break;
    case 6:  // Stance complete
        break;
    case 7:  // Open loop move error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        break;
    case 8:  // Closed loop move error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        break;
    case 9:  // Closed loop move complete
        break;
    case 10: // End of travel error
        setIntegerParam(pC_->motorStatusProblem_, 1);
		if (position > 0) {
            setIntegerParam(pC_->motorStatusHighLimit_, 1);
		} else {
            setIntegerParam(pC_->motorStatusLowLimit_, 1);
		}
        break;
    case 11: // Ramp move error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        break;
    default:
        break;
  }

  // Read the home status
  sprintf(pC_->outString_, "GHS");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Home status: 1"
  sscanf (pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  home = (replyValue == '1') ? 1:0;
  setIntegerParam(pC_->motorStatusAtHome_, home);

  // Read the drive power on status
  sprintf(pC_->outString_, "GPS");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Power supply enabled state: 1"
  sscanf (pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  driveOn = (replyValue == '1') ? 1:0;
  setIntegerParam(pC_->motorStatusPowerOn_, driveOn);
  setIntegerParam(pC_->motorStatusProblem_, 0);

  skip:
  setIntegerParam(pC_->motorStatusProblem_, comStatus ? 1:0);
  callParamCallbacks();
  return comStatus ? asynError : asynSuccess;
}

/** Code for iocsh registration */
static const iocshArg MD90CreateControllerArg0 = {"Port name", iocshArgString};
static const iocshArg MD90CreateControllerArg1 = {"MD-90 port name", iocshArgString};
static const iocshArg MD90CreateControllerArg2 = {"Number of axes", iocshArgInt};
static const iocshArg MD90CreateControllerArg3 = {"Moving poll period (ms)", iocshArgInt};
static const iocshArg MD90CreateControllerArg4 = {"Idle poll period (ms)", iocshArgInt};
static const iocshArg * const MD90CreateControllerArgs[] = {&MD90CreateControllerArg0,
                                                             &MD90CreateControllerArg1,
                                                             &MD90CreateControllerArg2,
                                                             &MD90CreateControllerArg3,
                                                             &MD90CreateControllerArg4};
static const iocshFuncDef MD90CreateControllerDef = {"MD90CreateController", 5, MD90CreateControllerArgs};
static void MD90CreateContollerCallFunc(const iocshArgBuf *args)
{
  MD90CreateController(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival);
}

static void MD90Register(void)
{
  iocshRegister(&MD90CreateControllerDef, MD90CreateContollerCallFunc);
}

extern "C" {
epicsExportRegistrar(MD90Register);
}
