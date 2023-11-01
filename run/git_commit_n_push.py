# This file does ...
#

import tac_utils

import os
import logging

logging.basicConfig(level="INFO")



print('\n')
name_of_this_file = os.path.basename(__file__)

logging.info( '----- ' + name_of_this_file + ' begin -----'  )

os.chdir( tac_utils.tac_root  )
cwd = os.getcwd()
logging.info( 'cwd: ' + cwd )

# todo... git submodeule for each commit push etc

logging.info( '----- ' + name_of_this_file + ' end -----'  )

