## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script
cd "/export/oslhome/ajf/WWWsite/genSubV1-6/iocBoot/iocgenSub"

< cdCommands
#< ../nfsCommands

cd topbin
## You may have to change genSub to something else
## everywhere it appears in this file

ld < genSub.munch

## This drvTS initializer is needed if the IOC has a hardware event system
#TSinit

## Register all support components
cd top
dbLoadDatabase("dbd/genSub.dbd",0,0)
genSub_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/dbReceive.db")

## Set this to see messages from mySub
#mySubDebug = 1

cd startup
iocInit()

## Start any sequence programs
#seq &sncExample,"user=ajf"
