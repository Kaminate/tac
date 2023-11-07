# This file sets a variable for the root tac directory to be used by other python scripts
# The root directory is checked first with the TAC_ROOT environment variable, and
# falls back to the parent directory of this file
# 
# usage:
#   import tac_utils
#   print( 'tac root: ', tac_utils.tac_root )
#   # prints 'tac_root: E:\Users\Nate\Documents\GitHub\tac
#
# For an example, see check_file_naming.py
#

import os
import logging
import subprocess
import importlib.util
import time

# --------------------------------------------------------------------------------------------------

# --------------------------------------------------------------------------------------------------

# Functions to install a python module

def PrintFile(file, state):
    basename = os.path.basename(file)
    logging.debug( '----- ' + basename + ' ' + state + ' -----'  )

def BeginFile(file):
    PrintFile(file, 'begin')
    return time.time()

def EndFile(file, beginTime):
    elapsedSecs = time.time() - beginTime
    PrintFile(file, 'end' + f' ({round(elapsedSecs,2)}s)')


def RunSubprocess( args ):
    logging.debug('> ' + ' '.join(args))
    subprocess.run(args)

def Uninstall(moduleName):
    RunSubprocess(["pip", "uninstall", moduleName, "-y"])

def IsInstalled(importName):
  spec = importlib.util.find_spec(importName)
  return spec is not None

def Install(importName, moduleName):
  if IsInstalled(importName):
    logging.debug(moduleName + ' is installed')
  else:
    RunSubprocess(["pip", "install", moduleName])


# --------------------------------------------------------------------------------------------------

def SetLogLevel( level ):

  logging.getLogger().setLevel( level )


# --------------------------------------------------------------------------------------------------

 # NOTSET, DEBUG, INFO, WARNING, ERROR, CRITICAL
logging.basicConfig(level="INFO")

beginTime = BeginFile(__file__)

# --------------------------------------------------------------------------------------------------

# set TAC_ROOT env var

tac_root_env_key = 'TAC_ROOT'
tac_root = os.getenv( tac_root_env_key )
if tac_root is None:
     
  cwd = os.getcwd(); # E:\Users\Nate\Documents\GitHub\tac\run
  tac_root = os.path.dirname( cwd ) # E:\Users\Nate\Documents\GitHub\tac
  os.environ[ tac_root_env_key ] = tac_root

  logging.debug( 'env TAC_ROOT undefined' )
  logging.debug( ['cwd: ', cwd] )
  logging.debug( ['setting TAC_ROOT to ', tac_root] )

def ChangeDirectoryToTacRoot():
  os.chdir( tac_root )

# --------------------------------------------------------------------------------------------------

# allow use of import git

Install( 'git', 'GitPython' ) 

# --------------------------------------------------------------------------------------------------

EndFile(__file__, beginTime)
