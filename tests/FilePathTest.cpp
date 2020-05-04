
#include <catch.hpp>

#include "filepath.h"
#include "platformutils.h"


TEST_CASE( "Filepath construction", "[filepath]" )
{
    std::string root = "D:";
    root.push_back(PlatformUtils::filePathSeparator);

    SECTION( "Creating from root" )
    {
        SECTION( "Arguments check" )
        {
            bool thrown;
            try{
                FilePath emptyPath("");
                thrown = false;
            } catch (std::exception& e) { thrown = true; }
            REQUIRE( thrown );
        }
        SECTION( "Root with slash" )
        {
            FilePath path(root);
            REQUIRE( path.getRoot() == root );
            REQUIRE( path.getPath() == root );
            REQUIRE( path.isDir() );
            REQUIRE( !path.canGoUp() );
        }
        SECTION( "Root without slash" )
        {
            auto root2 = root;
            root2.pop_back();
            FilePath path(root2);
            REQUIRE( path.getRoot() == root );
            REQUIRE( path.getPath() == root );
            REQUIRE( path.isDir() );
            REQUIRE( !path.canGoUp() );
        }
    }
    SECTION( "Creating from path and root" )
    {
        SECTION( "Arguments check" )
        {
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
        FilePath path(root);
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
            REQUIRE( !path.canGoUp() );
            REQUIRE( rootPath.compareTo(path) == FilePath::CompareResult::EQUAL );
        }
    }
}

TEST_CASE( "Filepath operations", "[filepath]" )
{
    std::string root = "D:";
    root.push_back(PlatformUtils::filePathSeparator);

    FilePath path(root);

    SECTION( "Adding dir/file" )
    {
        auto newPath = root;
        SECTION( "Add to dir path" )
        {
            REQUIRE( path.isDir() );

            SECTION( "Empty dir" )
            {
                bool canAddEmptyDir;
                try{
                    path.addDir("");
                    canAddEmptyDir = true;
                } catch (std::exception& e) { canAddEmptyDir = false; }
                REQUIRE( !canAddEmptyDir );
            }
            SECTION( "Non-empty dir" )
            {
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
            SECTION( "Empty file" )
            {
                bool canAddEmptyFile;
                try{
                    path.addFile("");
                    canAddEmptyFile = true;
                } catch (std::exception& e) { canAddEmptyFile = false; }
                REQUIRE( !canAddEmptyFile );
            }
            SECTION( "Non-empty file" )
            {
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
        }

        SECTION( "Add to file path" )
        {
            newPath.append("some_file");
            path.addFile("some_file");
            REQUIRE( !path.isDir() );

            SECTION( "Dir" )
            {
                bool canAddDirToFile;
                try{
                    path.addDir("test");
                    canAddDirToFile = true;
                } catch (std::exception& e) { canAddDirToFile = false; }
                REQUIRE( !canAddDirToFile );
            }
            SECTION( "File" )
            {
                bool canAddFileToFile;
                try{
                    path.addFile("test");
                    canAddFileToFile = true;
                } catch (std::exception& e) { canAddFileToFile = false; }
                REQUIRE( !canAddFileToFile );
            }
            //path didn't change
            REQUIRE( path.getName() == "some_file" );
            REQUIRE( path.getPath() == newPath );
        }
    }

    SECTION( "Path crcs" ) {
        FilePath newPath = path;
        path.addDir("test");
        // actually it could collide and be the same, but not with provided paths
        REQUIRE( path.getPathCrc() != newPath.getPathCrc() );
        newPath.addDir("test");
        REQUIRE( path.getPathCrc() == newPath.getPathCrc() );
        path.goUp();
        path.addFile("test2");
        REQUIRE( path.getPathCrc() != newPath.getPathCrc() );
        path.goUp();
        path.addFile("test");
        // path to dir/file with the same name has the same crc
        REQUIRE( path.getPathCrc() == newPath.getPathCrc() );
    }
    SECTION( "Path comparison" ) {
        FilePath newPath = path;

        SECTION( "Equal paths" )
        {
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::EQUAL );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::EQUAL );
            newPath.addDir("test");
            path.addDir("test");
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::EQUAL );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::EQUAL );
            newPath.addFile("test_file");
            path.addFile("test_file");
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::EQUAL );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::EQUAL );
        }
        SECTION( "Parent with child" )
        {
            newPath.addDir("test");
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::PARENT );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::CHILD );
        }
        SECTION( "Dir and file with equal names" )
        {
            newPath.addDir("test");
            path.addFile("test");
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::DIFFERENT );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::DIFFERENT );
        }
        SECTION( "Different paths" )
        {
            newPath.addDir("test");
            newPath.addFile("test");
            path.addFile("test2");
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::DIFFERENT );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::DIFFERENT );
            path.goUp();
            path.addFile("test");
            REQUIRE( path.compareTo(newPath) == FilePath::CompareResult::DIFFERENT );
            REQUIRE( newPath.compareTo(path) == FilePath::CompareResult::DIFFERENT );
        }
    }
    SECTION( "Make relative path" ) {
        FilePath rootPath("D:\\");
        SECTION( "Using correct root" )
        {
            std::string relPathStr = "Windows";
            relPathStr.push_back(PlatformUtils::filePathSeparator);
            relPathStr.append("System32");
            relPathStr.push_back(PlatformUtils::filePathSeparator);

            FilePath newPath("D:\\");
            newPath.addDir("Windows");
            newPath.addDir("System32");

            REQUIRE( newPath.makeRelativeTo(rootPath) );
            REQUIRE( newPath.getPath() == relPathStr );
            REQUIRE( newPath.goUp() );
            REQUIRE( newPath.getPath(false) == "Windows" );
        }
        SECTION( "Using different root" ) {
            FilePath newPath("C:\\");
            newPath.addDir("Windows");
            newPath.addDir("System32");
            auto prevPath = newPath.getPath();

            REQUIRE( !newPath.makeRelativeTo(path) );
            REQUIRE( newPath.getPath() == prevPath );
        }
    }
}

