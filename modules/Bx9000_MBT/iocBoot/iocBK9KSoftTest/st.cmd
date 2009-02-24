#!../../bin/linux-x86/BK9KSoftTest

## You may have to change BK9KSoftTest to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/BK9KSoftTest.dbd")
BK9KSoftTest_registerRecordDeviceDriver(pdbbase)

#***********************************************************************
###############  Hardware/Software Initialization ######################
#***********************************************************************
# ----------------------------------------------------------------------
# Initialize Beckhoff BK9000 Support
# ----------------------------------------------------------------------

# Add coupler
# Bx9000_Coupler_Add( UINT8 * cplrname, UINT8 * ipaddr, UINT8 * init_string)
Bx9000_Coupler_Add("ics-test-bh3","160.91.229.182",0)
# Add terminals to the coupler
# Bx9000_Terminal_Add( UINT8 * cplrname, UINT16 slot, UINT8 * btname, UINT8 * init_string)
Bx9000_Terminal_Add("ics-test-bh3", 1, "KL9505", 0)
Bx9000_Terminal_Add("ics-test-bh3", 2, "KL3314", 0)
Bx9000_Terminal_Add("ics-test-bh3", 3, "KL3312", 0)
Bx9000_Terminal_Add("ics-test-bh3", 4, "KL2408", 0)
Bx9000_Terminal_Add("ics-test-bh3", 5, "KL1104", 0)
Bx9000_Terminal_Add("ics-test-bh3", 6, "KL2622", 0)
Bx9000_Terminal_Add("ics-test-bh3", 7, "KL1408", 0)
Bx9000_Terminal_Add("ics-test-bh3", 8, "KL3102", 0)
Bx9000_Terminal_Add("ics-test-bh3", 9, "KL4132", 0)
Bx9000_Terminal_Add("ics-test-bh3", 10, "KL2124", 0)
Bx9000_Terminal_Add("ics-test-bh3", 11, "KL1124", 0)
Bx9000_Terminal_Add("ics-test-bh3", 12, "KL3064", 0)
Bx9000_Terminal_Add("ics-test-bh3", 13, "KL4002", 0)
Bx9000_Terminal_Add("ics-test-bh3", 14, "KL3468", 0)
Bx9000_Terminal_Add("ics-test-bh3", 15, "KL3408", 0)
Bx9000_Terminal_Add("ics-test-bh3", 16, "KL9010", 0)

#######################################################
#####   Load "Fully instantiated" databases here  #####
#######################################################
## Load record instances
cd ${TOP}/iocBoot/${IOC}
dbLoadTemplate("Bx9000Info-test.instance")
dbLoadTemplate("KL1124-test.instance")
dbLoadTemplate("KL2124-test.instance")
dbLoadTemplate("KL3064-test.instance")
dbLoadTemplate("KL3102-test.instance")
dbLoadTemplate("KL4132-test.instance")
dbLoadTemplate("KL4002-test.instance")
dbLoadTemplate("KL3312-test.instance")
dbLoadTemplate("KL3314-test.instance")
dbLoadTemplate("KL1408-test.instance")
dbLoadTemplate("KL2408-test.instance")
dbLoadTemplate("KL2622-test.instance")
dbLoadTemplate("KL1104-test.instance")
dbLoadTemplate("KL3468-test.instance")
dbLoadTemplate("KL3408-test.instance")
######################################################
### ---------- Database loading complete ----------###
#######################################################

## Set this to see messages from mySub
#var mySubDebug 1

cd ${TOP}/iocBoot/${IOC}
iocInit()

## Start any sequence programs
#seq sncExample,"user=pengHost"
