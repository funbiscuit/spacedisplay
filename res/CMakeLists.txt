
target_include_directories(spacedisplay PRIVATE build/include)

target_sources(spacedisplay PRIVATE build/src/resources.cpp)

if( WIN32 )
    target_sources(spacedisplay PRIVATE win-res.rc build/src/win.rc)
else()
    #TODO rewrite how objects are linked (make dynamic CMakeLists by resource builder)
    file(GLOB all_OBJS ${PROJECT_SOURCE_DIR}/res/build/objs/*.o)
    target_link_libraries(spacedisplay PRIVATE ${all_OBJS})
endif()
