#  -*- Makefile -*-
T_A=$EPICS_HOST_ARCH
TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# EDM Screens to install
# You can override the default installation path.
#INSTALL_LOCATION=

EDLS += save_restore.edl
#EDLS += save_restoreStatus.adl
#EDLS += save_restoreStatusLegend.adl
#EDLS += save_restoreStatus_more.adl
#EDLS += save_restoreStatus_tiny.adl
#EDLS += SR_X_Status.adl

# Install images to be used by EDM (Gif and Png are supported)
# PNG is better and preferred.
#PNGS +=
#GIFS +=

# EDM Screens to install
# If you want to install all screens and are
# lazy use the wildcard method. :)
#EDLS+=$(wildcard *.edl)
#PNGS+=$(wildcard *.png)
#GIFS+=$(wildcard *.gif)


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

realclean:
