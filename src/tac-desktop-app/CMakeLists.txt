add_library( TacDesktopApp )
set_target_properties( TacDesktopApp PROPERTIES FOLDER "Engine/Desktop App" )

add_subdirectory( desktop_app )
add_subdirectory( desktop_window )
add_subdirectory( desktop_event )
add_subdirectory( desktop_thread )
add_subdirectory( tac-win32 )

target_include_directories( TacDesktopApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ )
target_link_libraries( TacDesktopApp TacStdLib )

