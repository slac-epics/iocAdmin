#==============================================================
#
#  Abs:  Makefile to build EPICS components
#
#  Name: Makefile
#
#  Facility:  SLAC
#
#  Auth: 22-Jul-2005, Stephanie Allison   (SAA)
#  Rev:  dd-mmm-yyyy, Reviewer's Name     (USERNAME)
#
#--------------------------------------------------------------
#  Mod:
#       24-Jun-2008, B. Hall              (RDH)
#          Added ChannelArchiver for production network 
#       09-Mar-2007, J. Zhou              (JINGCHEN):
#          Added gateway, gnuregex (required by gateway)
#       07-Sep-2006, J. Zhou              (JINGCHEN):
#          Added ChannelArchiver
#       01-Mar-2006, K. Luchini           (LUCHINI):
#          Added alh
#       25-Jul-2005, K. Luchini           (LUCHINI):
#          Added vdct
#
#==============================================================
#
TOP = ..
include $(TOP)/configure/CONFIG

DIRS += edm
DIRS += ChannelWatcher
DIRS += StripTool
DIRS += alh
DIRS += km
DIRS += labca
DIRS += probe
DIRS += msi
DIRS += dbreport
DIRS += iocLogAndFwdServer
DIRS += ts
DIRS += gnuregex
DIRS += gateway
DIRS += ChannelArchiver
DIRS += vdct

include $(TOP)/configure/RULES_TOP
