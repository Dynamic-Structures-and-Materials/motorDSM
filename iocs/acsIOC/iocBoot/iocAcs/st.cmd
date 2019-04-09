#!../../bin/linux-x86_64/acs

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/acs.dbd"
acs_registerRecordDeviceDriver pdbbase

cd "${TOP}/iocBoot/${IOC}"


## motorUtil (allstop & alldone)
dbLoadRecords("$(MOTOR)/db/motorUtil.db", "P=acs:")

##

iocInit

## motorUtil (allstop & alldone)
motorUtilInit("acs:")

# Boot complete
