
#include <catch.hpp>
#include <iostream>
#include <sstream>

#include "filepath.h"
#include "platformutils.h"

// helper for redirecting cerr since FilePath may output some warnings when
// we do negative checking
struct CerrRedirect
{
    CerrRedirect(std::streambuf* new_buffer) : old(std::cerr.rdbuf(new_buffer)) {}

    ~CerrRedirect()
    {
        std::cerr.rdbuf(old);
    }

private:
    std::streambuf* old;
};

TEST_CASE( "Creating and editing filepath", "[filepath]" )
{
    std::stringstream cerrBuf;

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
        {
            CerrRedirect redirGuard(cerrBuf.rdbuf());
            REQUIRE( !path.addFile("some second file"));
        }
        REQUIRE( path.getName() == "some file" );
        REQUIRE( path.getPath() == newPath );
    }
    SECTION( "Creating and using empty path" ) {
        CerrRedirect redirGuard(cerrBuf.rdbuf());
        FilePath emptyPath("");
        REQUIRE( emptyPath.getPath().empty() );
        REQUIRE( emptyPath.getRoot().empty() );
        REQUIRE( emptyPath.getName().empty() );
        REQUIRE( emptyPath.getParts().empty() );
        REQUIRE( !emptyPath.addFile("any") );
        REQUIRE( !emptyPath.addDir("any") );
    }
    SECTION( "Creating from path and root. Arguments check." ) {
        bool thrown;
        try{
            FilePath newPath("", "");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( thrown );

        try{
            FilePath newPath("", "D:\\Windows");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( thrown );

        try{
            FilePath newPath("D:\\Windows", "");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( thrown );

        try{
            FilePath newPath("D:\\Windows", "C:\\");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( thrown );

        try{
            FilePath newPath("D:\\Windows", "D:\\Windows\\System32");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( thrown );

        try{
            //if path doesn't have slash at the end, it is considered to be a file
            //while root is always considered to be a directory
            FilePath newPath("D:\\Windows", "D:\\Windows\\");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( thrown );

        try{
            FilePath newPath("D:\\Windows\\", "D:\\Windows");
            FilePath newPath2("D:\\Windows\\", "D:\\Windows\\");
            FilePath newPath3("D:\\Windows", "D:\\");
            thrown = false;
        } catch (std::exception& e) { thrown = true; }
        REQUIRE( !thrown );

    }
    SECTION( "Creating from path and root" ) {
        SECTION( "Creating path to file" ) {
            path.addFile("Windows");
            FilePath filePath("D:\\Windows", "D:\\");
            REQUIRE( !filePath.isDir() );
            REQUIRE( filePath.compareTo(path) == FilePath::CompareResult::EQUAL );
            REQUIRE( filePath.goUp() );
            REQUIRE( !filePath.canGoUp() );
        }

        SECTION( "Creating path to directory" ) {
            path.addDir("Windows");
            FilePath dirPath("D:\\Windows\\", "D:\\");
            REQUIRE( dirPath.isDir() );
            REQUIRE( dirPath.compareTo(path) == FilePath::CompareResult::EQUAL );
            REQUIRE( dirPath.goUp() );
            REQUIRE( !dirPath.canGoUp() );
        }

        SECTION( "Creating path to root" ) {
            FilePath rootPath("D:\\", "D:\\");
            REQUIRE( rootPath.isDir() );
            REQUIRE( !rootPath.canGoUp() );
            REQUIRE( rootPath.compareTo(path) == FilePath::CompareResult::EQUAL );
        }
    }
    SECTION( "Check crcs work" ) {
        FilePath newPath("/home");
        auto rootCrc = path.getPathCrc();
        path.addDir("test");
        // actually it could collide and be the same, but not with provided paths
        REQUIRE( path.getPathCrc() != rootCrc );
        path.goUp();
        REQUIRE( path.getPathCrc() == rootCrc );
        path.addFile("test2");
        REQUIRE( path.getPathCrc() != rootCrc );
        path.setRoot(newPath.getRoot());
        REQUIRE( path.getPathCrc() == newPath.getPathCrc() );
    }
    SECTION( "Check compare work" ) {
        FilePath newPath("D:\\");
        REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::EQUAL );
        REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::EQUAL );
        newPath.addDir("test");
        REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::PARENT );
        REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::CHILD );
        path.addDir("test");
        REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::EQUAL );
        REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::EQUAL );
        path.goUp();
        path.addFile("test");
        REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::DIFFERENT );
        REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::DIFFERENT );
        newPath.goUp();
        newPath.addFile("test");
        REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::EQUAL );
        REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::EQUAL );
    }
    SECTION( "Check make relative" ) {
        std::string relPathStr = "Windows";
        relPathStr.push_back(PlatformUtils::filePathSeparator);
        relPathStr.append("System32");
        relPathStr.push_back(PlatformUtils::filePathSeparator);

        FilePath newPath("D:\\");
        newPath.addDir("Windows");
        newPath.addDir("System32");

        REQUIRE( newPath.makeRelativeTo(path) );
        REQUIRE( newPath.getPath() == relPathStr );
        REQUIRE( newPath.goUp() );
        REQUIRE( newPath.getPath(false) == "Windows" );
    }
    SECTION( "Check make relative with bad child" ) {
        FilePath newPath("C:\\");
        newPath.addDir("Windows");
        newPath.addDir("System32");
        auto prevPath = newPath.getPath();

        REQUIRE( !newPath.makeRelativeTo(path) );
        REQUIRE( newPath.getPath() == prevPath );
    }
}

