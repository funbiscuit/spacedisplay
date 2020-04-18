# link to generated resources
add_subdirectory(build)

# On windows add .rc file with app icon
if( WIN32 )
    target_sources(spacedisplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/win-res.rc)
endif()
