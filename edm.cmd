#!/bin/bash

# Setup edm environment
source /reg/g/pcds/setup/epicsenv-3.14.12.sh

# Launch edm with iocAdmin demo screen
edm -x -eolc -m "IOC=SXR:IOC:POLY,HOME=SXRHome.edl,HOST=ioc-sxr-motor" iocScreens/ioc_screen_demo.edl &
