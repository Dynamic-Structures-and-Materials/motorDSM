TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = acs
# acs.dbd will be created and installed
DBD += acs.dbd

# acs.dbd will be made up from these files:
acs_DBD += base.dbd

# Include dbd files from all support applications:
#acs_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#acs_LIBS += xxx

# acs_registerRecordDeviceDriver.cpp derives from acs.dbd
acs_SRCS += acs_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
acs_SRCS_DEFAULT += acsMain.cpp
acs_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#acs_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
acs_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

