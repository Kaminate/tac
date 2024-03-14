target_sources( TacECS
  PUBLIC
  "tac_client.h"
  "tac_server.h"
  "tac_space_net.h"
  PRIVATE
  "tac_client.cpp"
  "tac_server.cpp"
  "tac_space_net.cpp"
  "tac_entity_diff.cpp"
  "tac_entity_diff.h"
  "tac_player_diff.cpp"
  "tac_player_diff.h"
)

