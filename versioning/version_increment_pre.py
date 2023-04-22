""" Create version header and tracker file if missing """
import datetime
import os
from sys import exit

Import("env")
print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", BUILD_TARGETS)


#function to read short git sha
def get_git_short_sha():

    #get short sha
    sha = os.popen('git rev-parse --short HEAD').read().strip()

    #return short sha
    return sha

#function to check if working directory is clean
def is_git_clean():
    
        #get git status
        status = os.popen('git status --porcelain').read().strip()
    
        #return true if clean
        return not status

VERSION_HEADER = 'Version.h'


if not is_git_clean():
    print('Git working directory is not clean. Please commit all changes before building')
    if len(COMMAND_LINE_TARGETS) > 0 and COMMAND_LINE_TARGETS[0] == 'upload':
        exit(1)

VERSION = get_git_short_sha()

HEADER_FILE = """
// AUTO GENERATED FILE, DO NOT EDIT
#ifndef VERSION
    #define VERSION "{}"
#endif
#ifndef BUILD_TIMESTAMP
    #define BUILD_TIMESTAMP "{}"
#endif
""".format(VERSION, datetime.datetime.now())

if os.environ.get('PLATFORMIO_INCLUDE_DIR') is not None:
    VERSION_HEADER = os.environ.get('PLATFORMIO_INCLUDE_DIR') + os.sep + VERSION_HEADER
elif os.path.exists("include"):
    VERSION_HEADER = "include" + os.sep + VERSION_HEADER
else:
    PROJECT_DIR = env.subst("$PROJECT_DIR")
    os.mkdir(PROJECT_DIR + os.sep + "include")
    VERSION_HEADER = "include" + os.sep + VERSION_HEADER

with open(VERSION_HEADER, 'w+') as FILE:
    FILE.write(HEADER_FILE)

