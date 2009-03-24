## EVG Event Test IOC RTEMS startup script

setenv("EPICS_TS_NTP_INET", "134.79.16.9")
setenv("EPICS_CA_SERVER_PORT",   "5066")
setenv("EPICS_CA_REPEATER_PORT", "5067")
setenv("EPICS_IOC_LOG_PORT", "7004")
setenv("EPICS_IOC_LOG_INET", "134.79.219.12")

chdir("../../")
ld("bin/RTEMS-beatnik/eventTest.obj") 

dbLoadDatabase("dbd/eventTest.dbd")

eventTest_registerRecordDeviceDriver(pdbbase) 

# Load database(s) that corresponds to module(s) being configured.
dbLoadRecords("db/evgTest.db")
#dbLoadRecords("db/evrTest.db")

#ErDebug=100
#drvMrfErFlag=5
#drvMrfEgFlag=5

bspExtVerbosity=0

# Configure EVG if it exists.  
# Choose proper flag for internal clock.
#
#EgConfigure(0,0x280000,0)	# External Clock
EgConfigure(0,0x280000,1)	# Internal Clock
#
#    EgConfigure(<instance>,<address>,<internalClock>)
#
#    where instance = EVG instance, starting from 0, incrementing by 1
#                     for each subsequent card
#    and   address  = VME card address, starting from 0x280000, 
#                     incrementing by 0x100000 for each subsequent card
#    and   clock    = Internal clock flag:
#			0 = use external clock
#			1 = use internal clock

# Configure EVR if it exists.
# Choose the proper configuration - PMC or VME.
# Note - see README_evrTest for hardware setup and instructions for PMC 
# one-time configuration.
#
#ErConfigure(0,0x000000,0x00,0,1)  # PMC type
#ErConfigure(0,0x300000,0x60,4,0)  # VME type.
#
#    VME: ErConfigure(<instance>,<address>,<vector>,<level>,0)
#    PMC: ErConfigure(<instance>,    0    ,    0   ,   0   ,1)
#
#    where instance = EVR instance, starting from 0, incrementing by 1
#                     for each subsequent card
#    and   address  = VME card address, starting from 0x300000, 
#                     incrementing by 0x100000 for each subsequent card
#                     (0 for PMC)
#    and   vector   = VME interrupt vector, starting from 0x60, 
#                     incrementing by 0x02 for each subsequent card
#                     (0 for PMC)
#    and   level    = VME interrupt level (set to 4 - can be the same 
#                     for all EVRs)
#                     (0 for PMC)
#    and   0        = VME
#       or 1        = PMC

iocInit()
bspExtVerbosity=1
