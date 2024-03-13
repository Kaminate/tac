target_sources( TacStdLib
PUBLIC
  tac_array.h
  tac_fixed_vector.h
  tac_frame_vector.h
  tac_list.h
  tac_forward_list.h
  tac_map.h
  tac_optional.h
  tac_ring_array.h
  tac_ring_buffer.h
  tac_ring_vector.h
  tac_set.h
  tac_span.h
  tac_vector.h
PRIVATE
  tac_ring_buffer.cpp
  tac_list.cpp
  tac_forward_list.cpp
  tac_set.cpp
  tac_map.cpp
)

