#!../../bin/linux-x86_64/dsm

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/dsm.dbd"
dsm_registerRecordDeviceDriver pdbbase

cd "${TOP}/iocBoot/${IOC}"


## motorUtil (allstop & alldone)
dbLoadRecords("$(MOTOR)/db/motorUtil.db", "P=dsm:")

##

iocInit

## motorUtil (allstop & alldone)
motorUtilInit("dsm:")

# Boot complete
