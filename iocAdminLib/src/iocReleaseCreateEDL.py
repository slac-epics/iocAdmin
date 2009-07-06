#!/usr/bin/env python

import sys
import os
import subprocess
import optparse


__all__ = ['export_gui_file', 'module_versions', 'process_options']


def export_gui_file(module_versions, path=None):
    """
    Use the contents of a dictionary of module versions to create a database
    of module release stringin PVs. The database  
    is written to stdout if path is not provided or is None.
    """

    out_file = sys.stdout

    if path:
        try:
            out_file = open(path, 'w')
        except IOError, e:
            sys.stderr.write('Could not open "%s": %s\n' % (path, e.strerror))
            return None

    sorted_module_versions = [(key, module_versions[key]) for key in sorted(module_versions.keys())]   
    x = len(sorted_module_versions)
    height = 22 + x * 24   
    display_height = height + 76   
    """
    for 11 modules, height of box is 294 
    print "there are %d modules" % x
    print "height is %d" % height
    """

    """
    Write out the edl header
    """
    print >> out_file, '4 0 1'
    print >> out_file, 'beginScreenProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 0'
    print >> out_file, 'release 1'
    print >> out_file, 'x 75'
    print >> out_file, 'y 322'
    print >> out_file, 'w 500'
    print >> out_file, 'h %d' % display_height
    print >> out_file, 'font "helvetica-medium-r-12.0"'
    print >> out_file, 'ctlFont "helvetica-medium-r-12.0"'
    print >> out_file, 'btnFont "helvetica-medium-r-12.0"'
    print >> out_file, 'fgColor index 14'
    print >> out_file, 'bgColor index 7'
    print >> out_file, 'textColor index 14'
    print >> out_file, 'ctlFgColor1 index 16'
    print >> out_file, 'ctlFgColor2 index 14'
    print >> out_file, 'ctlBgColor1 index 4'
    print >> out_file, 'ctlBgColor2 index 12'
    print >> out_file, 'topShadowColor index 1'
    print >> out_file, 'botShadowColor index 11'
    print >> out_file, 'title "Module Versions $(ioc)"'
    print >> out_file, 'showGrid'
    print >> out_file, 'snapToGrid'
    print >> out_file, 'gridSize 8'
    print >> out_file, 'endScreenProperties'
    print >> out_file, ''
    print >> out_file, '# (Rectangle)'
    print >> out_file, 'object activeRectangleClass'
    print >> out_file, 'beginObjectProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 0'
    print >> out_file, 'release 0'
    print >> out_file, 'x 8'
    print >> out_file, 'y 64'
    print >> out_file, 'w 480'
    print >> out_file, 'h %d' % height
    print >> out_file, 'lineColor index 3'
    print >> out_file, 'fill'
    print >> out_file, 'fillColor index 3'
    print >> out_file, 'endObjectProperties'
    print >> out_file, ''
    print >> out_file, '# (Rectangle)'
    print >> out_file, 'object activeRectangleClass'
    print >> out_file, 'beginObjectProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 0'
    print >> out_file, 'release 0'
    print >> out_file, 'x 0'
    print >> out_file, 'y 0'
    print >> out_file, 'w 500'
    print >> out_file, 'h 40'
    print >> out_file, 'lineColor index 53'
    print >> out_file, 'fill'
    print >> out_file, 'fillColor index 53'
    print >> out_file, 'lineWidth 0'
    print >> out_file, 'endObjectProperties'
    print >> out_file, ''
    print >> out_file, '# (Static Text)'
    print >> out_file, 'object activeXTextClass'
    print >> out_file, 'beginObjectProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 1'
    print >> out_file, 'release 0'
    print >> out_file, 'x 8'
    print >> out_file, 'y 0'
    print >> out_file, 'w 352'
    print >> out_file, 'h 40'
    print >> out_file, 'font "helvetica-medium-r-12.0"'
    print >> out_file, 'fgColor index 14'
    print >> out_file, 'bgColor index 53'
    print >> out_file, 'useDisplayBg'
    print >> out_file, 'value {'
    print >> out_file, '  "LCLS IOC:"'
    print >> out_file, '  "$(ioc)"'
    print >> out_file, '}'
    print >> out_file, 'endObjectProperties'
    print >> out_file, ''
    print >> out_file, '# (Shell Command)'
    print >> out_file, 'object shellCmdClass'
    print >> out_file, 'beginObjectProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 2'
    print >> out_file, 'release 0'
    print >> out_file, 'x 352'
    print >> out_file, 'y 8'
    print >> out_file, 'w 48'
    print >> out_file, 'h 24'
    print >> out_file, 'fgColor index 14'
    print >> out_file, 'bgColor index 4'
    print >> out_file, 'topShadowColor index 1'
    print >> out_file, 'botShadowColor index 11'
    print >> out_file, 'font "helvetica-medium-r-12.0"'
    print >> out_file, 'buttonLabel "Print..."'
    print >> out_file, 'numCmds 0'
    print >> out_file, 'endObjectProperties'
    print >> out_file, ''
    print >> out_file, '# (Exit Button)'
    print >> out_file, 'object activeExitButtonClass'
    print >> out_file, 'beginObjectProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 1'
    print >> out_file, 'release 0'
    print >> out_file, 'x 424'
    print >> out_file, 'y 8'
    print >> out_file, 'w 48'
    print >> out_file, 'h 24'
    print >> out_file, 'fgColor index 14'
    print >> out_file, 'bgColor index 4'
    print >> out_file, 'topShadowColor index 1'
    print >> out_file, 'botShadowColor index 11'
    print >> out_file, 'label "Exit"'
    print >> out_file, 'font "helvetica-medium-r-12.0"'
    print >> out_file, '3d'
    print >> out_file, 'endObjectProperties'
    print >> out_file, ''
    print >> out_file, '# (Static Text)'
    print >> out_file, 'object activeXTextClass'
    print >> out_file, 'beginObjectProperties'
    print >> out_file, 'major 4'
    print >> out_file, 'minor 1'
    print >> out_file, 'release 0'
    print >> out_file, 'x 8'
    print >> out_file, 'y 56'
    print >> out_file, 'w 240'
    print >> out_file, 'h 16'
    print >> out_file, 'font "helvetica-medium-r-12.0"'
    print >> out_file, 'fontAlign "center"'
    print >> out_file, 'fgColor index 14'
    print >> out_file, 'bgColor index 3'
    print >> out_file, 'value {'
    print >> out_file, '  "Module versions used by this application"'
    print >> out_file, '}'
    print >> out_file, 'endObjectProperties'
    print >> out_file, ''

    module_y = 80
    for [key, module_version] in sorted_module_versions:
        x = key.replace("_MODULE_VERSION","",1)
        """
        Make a label for each module 
        """
        print >> out_file, '# (Static Text)'
        print >> out_file, 'object activeXTextClass'
        print >> out_file, 'beginObjectProperties'
        print >> out_file, 'major 4'
        print >> out_file, 'minor 1'
        print >> out_file, 'release 0'
        print >> out_file, 'x 16'
        print >> out_file, 'y %d' % module_y
        print >> out_file, 'w 144'
        print >> out_file, 'h 16'
        print >> out_file, 'font "helvetica-medium-r-12.0"'
        print >> out_file, 'fgColor index 14'
        print >> out_file, 'bgColor index 3'
        print >> out_file, 'value {'
        print >> out_file, '  "%s"' % x 
        print >> out_file, '}'
        print >> out_file, 'endObjectProperties'
        print >> out_file, ''
        module_y = module_y + 24 

    module_y = 80
    for [key, module_version] in sorted_module_versions:
        x = key.replace("_MODULE_VERSION","",1)
        """
        Make a textupdate widget for each module
        """
        print >> out_file, '# (Textupdate)'
        print >> out_file, 'object TextupdateClass'
        print >> out_file, 'beginObjectProperties'
        print >> out_file, 'major 10'
        print >> out_file, 'minor 0'
        print >> out_file, 'release 0'
        print >> out_file, 'x 180'
        print >> out_file, 'y %d' % module_y
        print >> out_file, 'w 152'
        print >> out_file, 'h 24'
        print >> out_file, 'controlPv "$(ioc):%s.VAL"' % x
        print >> out_file, 'fgColor index 16'
        print >> out_file, 'fgAlarm'
        print >> out_file, 'bgColor index 12'
        print >> out_file, 'fill'
        print >> out_file, 'font "helvetica-medium-r-12.0"'
        print >> out_file, 'fontAlign "center"'
        print >> out_file, 'lineWidth 2'
        print >> out_file, 'lineAlarm'
        print >> out_file, 'endObjectProperties'
        print >> out_file, ''
        module_y = module_y + 24
    
    if out_file != sys.stdout:
        out_file.close()
    

def module_versions(path):
    """
    Return a dictionary containing module names and versions.
    """
    
    try:
        release_file = open(path, 'r')
    except IOError, e:
        sys.stderr.write('Could not open "%s": %s\n' % (path, e.strerror))
        return None

    release_file_dict = {}

    for line in release_file:
        # Remove comments
        line = line.partition('#')[0]
        
        # Turn 'a = b' into a key/value pair and remove leading and trailing whitespace
        (key, sep, value) = line.partition('=')
        key = key.strip()
        value = value.strip()
        
        # Add the key/value pair to the dictionary if the key ends with _MODULE_VERSION
        if key.endswith('_MODULE_VERSION'):
            release_file_dict[key] = value
    
    release_file.close()
        
    return release_file_dict


def process_options(argv):
    """
    Return parsed command-line options found in the list of
    arguments, `argv`, or ``sys.argv[1:]`` if `argv` is `None`.
    """

    if argv is None:
        argv = sys.argv[1:]

    usage = 'Usage: %prog RELEASE_FILE [options]'    
    version = '%prog 0.1'
    parser = optparse.OptionParser(usage=usage, version=version)

    parser.add_option('-v', '--verbose', action='store_true', dest='verbose', help='print verbose output')
    parser.add_option("-e", "--db_file", action="store", type="string", dest="db_file", metavar="FILE", help="module database file path")

    parser.set_defaults(verbose=False,
                        db_file=None)

    (options, args) = parser.parse_args(argv)

    if len(args) != 1:
        parser.error("incorrect number of arguments")

    options.release_file_path = os.path.normcase(args[0])

    return options 


def main(argv=None):
    options = process_options(argv)

    versions = module_versions(options.release_file_path)
    export_gui_file(versions, options.db_file)

    return 0
    

if __name__ == '__main__':
    status = main()
    sys.exit(status)
