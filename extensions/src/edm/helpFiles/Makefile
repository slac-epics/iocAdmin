TOP = ../../..
include $(TOP)/configure/CONFIG

# Configuration files
vpath % $(ALL_SRC_DIRS)
INSTALL_DOC = $(INSTALL_LOCATION)/helpFiles/edm
DOCS += $(notdir $(wildcard ../*.htm))

# Configuration files
vpath style.css
INSTALL_TEMPLATES = $(INSTALL_LOCATION)/templates/edm
TEMPLATES += style.css

# EDM screens help files
INSTALL_EDL = $(INSTALL_LOCATION)/helpFiles/edm
EDLS += $(notdir $(wildcard ../*.edl))
INSTALL_GIF = $(INSTALL_LOCATION)/helpFiles/edm
GIFS += $(notdir $(wildcard ../*.gif))
INSTALL_PNG = $(INSTALL_LOCATION)/helpFiles/edm
PNGS += $(notdir $(wildcard ../*.png))

include $(TOP)/configure/RULES
