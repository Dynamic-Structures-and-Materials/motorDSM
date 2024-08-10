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
#include <chrono>
#include <thread>

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

/** Print out message if the motor controller returns a non-zero error code
  * \param[in] functionName  The function originating the call
  * \param[in] reply         Reply message returned from motor controller
  */
asynStatus MD90Axis::parseReply(const char *functionName, const char *reply)
{
  int replyStatus;
  char replyString[256];
  int replyValue;
  asynStatus comStatus;

  comStatus = asynSuccess;

  if (reply[0] == '\0') {
    comStatus = asynError;
	replyStatus = -1;
  } else if ( strcmp(reply, "Unrecognized command.") == 0 ) {
    replyStatus = 6;
  } else {
    sscanf(reply, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  }

  if (replyStatus != 0) {
    asynPrint(pasynUser_, ASYN_TRACE_ERROR,
      "%s:  %s\n",
      functionName, reply);
  }

  return comStatus;
}

/** Acceleration currently unsupported with MD-90 controller
  * \param[in] acceleration  The accelerations to ramp up to max velocity
  * \param[in] velocity      Motor velocity in steps / sec
  */
asynStatus MD90Axis::sendAccelAndVelocity(double acceleration, double velocity) 
{
  asynStatus status;
  int freq;
  static const char *functionName = "MD90::sendAccelAndVelocity";

  // Send the velocity
  // Velocity provided in steps/sec
  // Our unit step size of the encoder is 10 nm, but the motor moves in steps approx. 10 micrometers.
  // Motor controller accepts step frequency in Hz.
  freq = NINT(fabs(velocity / COUNTS_PER_STEP));
  sprintf(pC_->outString_, "SSF %d", freq);
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }

  return status;
}


asynStatus MD90Axis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status;
  static const char *functionName = "MD90Axis::move";

  status = sendAccelAndVelocity(acceleration, maxVelocity);
  
  // Position specified in encoder steps (10 nm), but motor move commands are in nanometers
  position = position * 10;
  if (relative) {
    sprintf(pC_->outString_, "CRM %d", NINT(position));
  } else {
    sprintf(pC_->outString_, "CLM %d", NINT(position));
  }
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
  return status;
}

asynStatus MD90Axis::home(double minVelocity, double maxVelocity, double acceleration, int forwards)
{
  int sleepTime;
  asynStatus status;
  static const char *functionName = "MD90Axis::home";

  status = sendAccelAndVelocity(acceleration, maxVelocity);

  // The MD-90 will start the home routine in the direction of the last move
  // Here we first make a small move to set the desired direction before homing

  if (!status) {
    sprintf(pC_->outString_, "SNS %d", SMALL_NSTEPS);
    status = pC_->writeReadController();
  }
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }

  if (forwards) {
    sprintf(pC_->outString_, "ESF");
  } else {
    sprintf(pC_->outString_, "ESB");
  }
  if (!status) {
    status = pC_->writeReadController();
  }
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }

  if (!status) {
    // Wait for the move to complete, then home
    sleepTime = SLEEP_MARGIN * SMALL_NSTEPS * COUNTS_PER_STEP / maxVelocity;
    if (sleepTime < HOME_SLEEP_MIN) {
        sleepTime = HOME_SLEEP_MIN;
    }
    std::this_thread::sleep_for(std::chrono::seconds(sleepTime));

    sprintf(pC_->outString_, "HOM");
    status = pC_->writeReadController();
  }
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
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

  /* MD-90 does not have jog command. Move max 6000 steps */
  sprintf(pC_->outString_, "SNS 6000");
  status = pC_->writeReadController();
  if (maxVelocity > 0.) {
    /* This is a positive move in MD90 coordinates */
    sprintf(pC_->outString_, "ESF");
  } else {
    /* This is a negative move in MD90 coordinates */
    sprintf(pC_->outString_, "ESB");
  }
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
  return status;
}

asynStatus MD90Axis::stop(double acceleration )
{
  asynStatus status;
  static const char *functionName = "MD90Axis::stop";

  sprintf(pC_->outString_, "STP");
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
  return status;
}

/** The ACS driver used this to turn on/off the motor winding current, so
  * we'll use this for enabling/disabling the persistent move state.
  */
asynStatus MD90Axis::setClosedLoop(bool closedLoop)
{
  asynStatus status;
  static const char *functionName = "MD90Axis::setClosedLoop";

  if (closedLoop == 1) {
    sprintf(pC_->outString_, "EPM");
  } else {
    sprintf(pC_->outString_, "DPM");
  }
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
  return status;
}

/** Set the I Gain of the motor control loop.  The motor is an I- controller
  * and has no P or D terms.
  * \param[in] iGain The current I gain in the control loop
  */
asynStatus MD90Axis::setIGain(double iGain)
{
  asynStatus status;
  static const char *functionName = "MD90Axis::setIGain";

  iGain = iGain * 1000;
  if (iGain < 1) iGain = 1.0;
  if (iGain > 1000) iGain = 1000.0;
  sprintf(pC_->outString_, "SGN %d", NINT(iGain));
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
  return status;
}

asynStatus MD90Axis::doMoveToHome()
{
  asynStatus status;
  static const char *functionName = "MD90Axis::doMoveToHome";

  sprintf(pC_->outString_, "CLM 0");
  status = pC_->writeReadController();
  if (!status) {
    status = parseReply(functionName, pC_->inString_);
  }
  return status;
}

/** Polls the axis.
  * This function reads the motor position, the limit status, the home status, the moving status, 
  * and the drive power-on status. 
  * It calls setIntegerParam() and setDoubleParam() for each item that it polls,
  * and then calls callParamCallbacks() at the end.
  * \param[out] moving A flag that is set indicating that the axis is moving (true) or done (false).
  */
asynStatus MD90Axis::poll(bool *moving)
{ 
  int replyStatus;
  char replyString[256];
  int replyValue;
  int done;
  int driveOn;
  int homed;
  double position;
  double velocity;
  asynStatus comStatus;
  static const char *functionName = "MD90Axis::poll";

  // TODO:  Will need to add some more error handling for the motor return codes.

  setIntegerParam(pC_->motorStatusProblem_, 0);

  // Read the drive power on status
  sprintf(pC_->outString_, "GPS");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Power supply enabled state: 1"
  sscanf(pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  driveOn = (replyValue == 1) ? 1:0;
  setIntegerParam(pC_->motorStatusPowerOn_, driveOn);

  // Read the home status
  sprintf(pC_->outString_, "GHS");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Home status: 1"
  sscanf(pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  homed = (replyValue == 1) ? 1:0;
  setIntegerParam(pC_->motorStatusHomed_, homed);

  // Read the moving status of this motor
  sprintf(pC_->outString_, "STA");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Current status value: 0"
  sscanf(pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  done = (replyValue == 2) ? 0:1;
  setIntegerParam(pC_->motorStatusDone_, done);
  *moving = done ? false:true;
  switch(replyValue) {
    case 0:  // Idle
        asynPrint(pasynUser_, ASYN_TRACE_FLOW, "%s:  Idle\n", functionName);
        break;
    case 1:  // Open loop move complete
        asynPrint(pasynUser_, ASYN_TRACE_FLOW, "%s:  Open loop move complete\n", functionName);
        break;
    case 2:  // Move in progress
        asynPrint(pasynUser_, ASYN_TRACE_FLOW, "%s:  Move in progress\n", functionName);
        break;
    case 3:  // Move stopped
        asynPrint(pasynUser_, ASYN_TRACE_FLOW, "%s:  Move stopped\n", functionName);
        break;
    case 4:  // Homing error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:  Homing error\n", functionName);
        break;
    case 5:  // Stance error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:  Stance error (Error resetting pose during closed loop move)\n", functionName);
        break;
    case 6:  // Stance complete
        asynPrint(pasynUser_, ASYN_TRACE_FLOW, "%s:  Stance complete (Finished resetting pose, starting extension move)\n", functionName);
        break;
    case 7:  // Open loop move error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:  Open loop move error\n", functionName);
        break;
    case 8:  // Closed loop move error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:  Closed loop move error\n", functionName);
        break;
    case 9:  // Closed loop move complete
        asynPrint(pasynUser_, ASYN_TRACE_FLOW, "%s:  Closed loop move complete\n", functionName);
        break;
    case 10: // End of travel error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:  End of travel error\n", functionName);
		if (position > 0) {
            setIntegerParam(pC_->motorStatusHighLimit_, 1);
		} else {
            setIntegerParam(pC_->motorStatusLowLimit_, 1);
		}
        break;
    case 11: // Ramp move error
        setIntegerParam(pC_->motorStatusProblem_, 1);
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:  Ramp move error\n", functionName);
        break;
    default:
        break;
  }

  // Read the current motor position in encoder steps (10 nm)
  sprintf(pC_->outString_, "GEC");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Current position in encoder counts: 1000"
  sscanf(pC_->inString_, "%d: %[^:]: %lf", &replyStatus, replyString, &position);
  setDoubleParam(pC_->motorPosition_, position);
  setDoubleParam(pC_->motorEncoderPosition_, position);
  setIntegerParam(pC_->motorStatusAtHome_, (position == 0) ? 1:0); // home limit switch
  setIntegerParam(pC_->motorStatusHome_, (position == 0) ? 1:0); // at home position

  // Read the current motor step frequency to calculate approx. set velocity in (encoder step lengths / s)
  sprintf(pC_->outString_, "GSF");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Current step frequency: 100"
  sscanf(pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  velocity = replyValue * COUNTS_PER_STEP;
  setDoubleParam(pC_->motorVelocity_, velocity);

  // Read the current motor integral gain (range 1-1000)
  sprintf(pC_->outString_, "GGN");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Gain: 1000"
  sscanf(pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  setDoubleParam(pC_->motorIGain_, replyValue);

  // Read the current motor persistent move state (using EPICS motorClosedLoop to report this)
  sprintf(pC_->outString_, "GPM");
  comStatus = pC_->writeReadController();
  if (comStatus) goto skip;
  // The response string is of the form "0: Current persistent move state: 1"
  sscanf(pC_->inString_, "%d: %[^:]: %d", &replyStatus, replyString, &replyValue);
  setIntegerParam(pC_->motorClosedLoop_, (replyValue == 0) ? 0:1);

  // set some default params
  setIntegerParam(pC_->motorStatusHasEncoder_, 1);
  setIntegerParam(pC_->motorStatusGainSupport_, 1);

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
