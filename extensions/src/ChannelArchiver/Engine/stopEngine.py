#!/usr/bin/env python
# $Id: stopEngine.py,v 1.1.1.1 2004/04/01 20:49:33 luchini Exp $
# Simple example how engine can be stopped via python.
# - And since python can be called from a script:
#   Example how to influence the engine from scripts ;-)
# -kuk-
#
# This implementation exists thanks to Noboru from KEK

##############################
import urllib, sys, regex

# Configure this:
#
# leave user empty if you do not use the user/passwd mechanism
#       user="engine",

def stop(machine="localhost",port="4812", user="",password=""):
    if user:
        url="http://%s:%s/stop?USER=%s&PASS=%s" \
             % (machine, port, user, password)
    else:
        url="http://%s:%s/stop" % (machine, port)
    try:
        url=urllib.urlopen (url)
    except IOError, message:
        print "Cannot connect to %s, port %s: error %s"%(machine, port, message)
        sys.exit (-1)
        
    page=url.read()
    if regex.search ("Engine Stopped", page) >= 0:
        print "Engine stopped"
    else:
        print "Cannot stop Engine, this is the raw page I got:"
        print page

if __name__ == "__main__":
    from sys import argv
    import getopt,re
    
    if len(argv) <= 1:
        stop()
    else:
        opts, pargs= getopt.getopt(argv[1:],
                                   "h:",
                                   ["machine=","user=","password=",
                                    "port=","help"])
        kw={}
        for key,val in opts:
            key=re.sub("-+","",key)
            kw[key]=val
        if kw.has_key("help"):
            print "Usage: %s [-h hostname] [--machine machine] [--port port] [--user username] [--password password]" % argv[0]
            sys.exit()
        if kw.has_key("h") and not kw.has_key("machine"):
            kw["machine"]=kw["h"]
            del kw["h"]
        apply(stop,(),kw)

