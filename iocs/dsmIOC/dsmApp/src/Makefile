TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following are used for debugging messages.
#!USR_CXXFLAGS += -DDEBUG

#=============================
# Build the IOC application

PROD_IOC = dsm
# dsm.dbd will be created and installed
DBD += dsm.dbd

# dsm.dbd will be made up from these files:
dsm_DBD += base.dbd

# Include dbd files from all support applications:
#ifdef ASYN
dsm_DBD += asyn.dbd
dsm_DBD += drvAsynSerialPort.dbd
dsm_DBD += drvAsynIPPort.dbd
#endif
dsm_DBD += motorSupport.dbd
dsm_DBD += devDsmMotor.dbd

# Add all the support libraries needed by this IOC
dsm_LIBS += dsm
dsm_LIBS += motor
#ifdef ASYN
dsm_LIBS += asyn
#endif
#ifdef SNCSEQ
dsm_LIBS += seq pv
#endif

# dsm_registerRecordDeviceDriver.cpp derives from dsm.dbd
dsm_SRCS += dsm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
dsm_SRCS_DEFAULT += dsmMain.cpp
dsm_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#dsm_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
dsm_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

