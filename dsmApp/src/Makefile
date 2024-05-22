# Makefile
TOP = ../..
include $(TOP)/configure/CONFIG

# The following are used for debugging messages.
USR_CXXFLAGS += -DDEBUG

DBD += devDsmMotor.dbd

LIBRARY_IOC = dsm

SRCS += dsmRegister.cc

# Advanced Control Systems driver support.
SRCS += devMCB4B.c drvMCB4B.c
SRCS += MCB4BDriver.cpp

dsm_LIBS += motor asyn
dsm_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES

