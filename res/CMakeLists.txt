# link to generated resources
add_subdirectory(build)
target_link_libraries(spacedisplay PRIVATE SpacedisplayRes::SpacedisplayRes)

# On windows add .rc file with app icon
if( WIN32 )
    target_sources(spacedisplay PRIVATE win-res.rc)
endif()
