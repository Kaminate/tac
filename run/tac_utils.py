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

tac_root_env_key = 'TAC_ROOT'

logging.basicConfig(level="INFO")

tac_root = os.getenv( tac_root_env_key )
if tac_root is None:
     
  logging.info( '----- tac_utils.py begin -----'  )

  # cwd      = E:\Users\Nate\Documents\GitHub\tac\run
  cwd =  os.getcwd();
  logging.info( 'env TAC_ROOT undefined' )
  logging.info( ['cwd: ', cwd] )

  # tac_root = E:\Users\Nate\Documents\GitHub\tac
  # os.path.dirname returns 1st result of os.path.split
  tac_root = os.path.dirname( os.getcwd() )

  logging.info( ['setting TAC_ROOT to ', tac_root] )

  os.environ[ tac_root_env_key ] = tac_root

  logging.info( '----- tac_utils.py end -----' )



