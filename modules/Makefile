# PCDS Specific Makefile!
# This Makefile is not part of EPICS, it is only there to support
# a make of the full repository from the top

include $(EPICS_SITE_TOP)/RELEASE_SITE

DIRS=
DIRS+=restore
DIRS+=miscUtils
DIRS+=seq
DIRS+=iocAdmin
DIRS+=generalTime
DIRS+=sSubRecord
DIRS+=event
DIRS+=genSub
DIRS+=ipac
DIRS+=asyn
DIRS+=ModBusTCPClnt
DIRS+=Bx9000_MBT
DIRS+=snmp
DIRS+=EDT_CL
DIRS+=autosave
DIRS+=highlandLVDTV550
DIRS+=ip231
DIRS+=motor
DIRS+=streamdevice
DIRS+=ether_ip
DIRS+=hytecMotor8601
DIRS+=ip330
DIRS+=xy2440
DIRS+=ics121
DIRS+=xy2445
DIRS+=VHSx0x
DIRS+=ics130
#DIRS+=modbus

all:

%:
	@set -e; for dir in $(DIRS); do \
		echo "=========== Running $@ in module $${dir} ==========="; \
		make -C $${dir} $@; \
	done
