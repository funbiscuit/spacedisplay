
target_include_directories(spacedisplay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(spacedisplay PRIVATE
        colortheme.h colortheme.cpp fileentry.h fileentry.cpp fileentrypool.h fileentrypool.cpp
        fileentrypopup.h fileentrypopup.cpp fileentryview.h fileentryview.cpp filepath.h filepath.cpp
        mainwindow.h mainwindow.cpp platformutils.h resources.h resources.cpp spacescanner.h spacescanner.cpp
        spaceview.h spaceview.cpp statusview.h statusview.cpp utils.h utils.cpp main.cpp
        )

if(WIN32)
    target_sources(spacedisplay PRIVATE platformutils-win.cpp)
else()
    target_sources(spacedisplay PRIVATE platformutils-nix.cpp)
endif()
