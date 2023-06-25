# TODO: write about the purpose of this file, and
# what relationship tac has with python scripts
import os

tac_root_env_key = 'TAC_ROOT'

tac_root = os.getenv( tac_root_env_key )
if tac_root is None:
  # os.path.dirname returns 1st result of os.path.split
  tac_root = os.path.dirname( os.getcwd() )

  os.environ[ tac_root_env_key ] = tac_root



