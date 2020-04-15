
target_include_directories(spacedisplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(spacedisplay PRIVATE
        colortheme.cpp fileentry.cpp fileentrypool.cpp
        fileentrypopup.cpp fileentryview.cpp filepath.cpp
        mainwindow.cpp resources.cpp spacescanner.cpp
        spaceview.cpp statusview.cpp utils.cpp main.cpp
        )

if(WIN32)
    target_sources(spacedisplay PRIVATE platformutils-win.cpp)
else()
    target_sources(spacedisplay PRIVATE platformutils-nix.cpp)
endif()
