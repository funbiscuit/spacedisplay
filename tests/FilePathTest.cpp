
#include <catch.hpp>

#include "filepath.h"
#include "platformutils.h"


TEST_CASE( "Creating and editing filepath", "[filepath]" )
{
    std::string root = "D:";
    root.push_back(PlatformUtils::filePathSeparator);

    FilePath path(root);
    REQUIRE( path.getRoot() == root );
    REQUIRE( path.getPath() == root );
    REQUIRE( path.isDir() );

    SECTION( "Can't go up when at root" ) {
        REQUIRE( !path.canGoUp() );
        REQUIRE( !path.goUp() );
    }
    SECTION( "Adding directory" ) {
        auto newPath = root;
        newPath.append("some dir");
        REQUIRE( path.addDir("some dir"));
        REQUIRE( path.canGoUp() );
        REQUIRE( path.isDir() );
        REQUIRE( path.getName() == "some dir" );
        REQUIRE( path.getPath(false) == newPath );
        newPath.push_back(PlatformUtils::filePathSeparator);
        REQUIRE( path.getPath(true) == newPath );
        REQUIRE( path.goUp() );
        REQUIRE( !path.canGoUp() );
    }
    SECTION( "Adding file" ) {
        auto newPath = root;
        newPath.append("some file");
        REQUIRE( path.addFile("some file"));
        REQUIRE( path.canGoUp() );
        REQUIRE( !path.isDir() );
        REQUIRE( path.getName() == "some file" );
        REQUIRE( path.getPath(false) == newPath );
        REQUIRE( path.getPath(true) == newPath );
        REQUIRE( path.goUp() );
        REQUIRE( !path.canGoUp() );
    }
    SECTION( "Adding file twice" ) {
        auto newPath = root;
        newPath.append("some file");
        REQUIRE( path.addFile("some file"));
        REQUIRE( !path.addFile("some second file"));
        REQUIRE( path.getName() == "some file" );
        REQUIRE( path.getPath() == newPath );
    }
    SECTION( "Creating and using empty path" ) {
        FilePath emptyPath("");
        REQUIRE( emptyPath.getPath().empty() );
        REQUIRE( emptyPath.getRoot().empty() );
        REQUIRE( emptyPath.getName().empty() );
        REQUIRE( emptyPath.getParts().empty() );
        REQUIRE( !emptyPath.addFile("any") );
        REQUIRE( !emptyPath.addDir("any") );
    }
}

