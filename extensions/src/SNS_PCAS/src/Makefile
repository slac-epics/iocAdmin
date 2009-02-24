TOP=../../..

include $(TOP)/configure/CONFIG

#=============================
# build executables

PROD_HOST += readPV
PROD_HOST += writePV
PROD_HOST += sns_cas

# Use CAS diagnostics statistics
USR_CXXFLAGS += -DCAS_DIAGNOSTICS

readPV_SRCS += readPV.cc
writePV_SRCS += writePV.cc
sns_cas_SRCS += main.cc
sns_cas_SRCS += exServer.cc
sns_cas_SRCS += exPV.cc
sns_cas_SRCS += exVectorPV.cc
sns_cas_SRCS += exScalarPV.cc
sns_cas_SRCS += exChannel.cc
sns_cas_SRCS += exParser.cc
readPV_LIBS += $(EPICS_BASE_IOC_LIBS)
writePV_LIBS += $(EPICS_BASE_IOC_LIBS)
sns_cas_LIBS += cas gdd
sns_cas_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE

