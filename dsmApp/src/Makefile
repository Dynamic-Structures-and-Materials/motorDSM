# Makefile
TOP = ../..
include $(TOP)/configure/CONFIG

# The following are used for debugging messages.
USR_CXXFLAGS += -DDEBUG

DBD += devDsmMotor.dbd

LIBRARY_IOC = dsm

SRCS += dsmRegister.cc

# Advanced Control Systems driver support.
SRCS += devMD90.c drvMD90.c
SRCS += MD90Driver.cpp

dsm_LIBS += motor asyn
dsm_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

