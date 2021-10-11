#include <iostream>
#include <fstream>

#include "filepath.h"
#include "spacewatcher.h"
#include "platformutils.h"
#include "DirHelper.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("SpaceWatcher basic events", "[watcher]")
{
    std::unique_ptr<SpaceWatcher> watcher;

    SECTION("Can't watch not existing path") {
        REQUIRE_THROWS_AS(SpaceWatcher::create("not existing dir"), std::runtime_error);
    }

    //wait for watcher thread to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    FilePath root("TestDir");
    DirHelper dh(root.getPath());

    SECTION("Create directory or file")
    {
        REQUIRE_NOTHROW(watcher = SpaceWatcher::create(root.getPath()));
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        root.addDir("test");
        dh.createDir("test");
        watcher->addDir(root.getPath());

        //wait for watcher thread to detect events
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        auto ev = watcher->popEvent();
        REQUIRE(ev != nullptr);

        if (ev->filepath.back() != PlatformUtils::filePathSeparator)
            ev->filepath.push_back(PlatformUtils::filePathSeparator);
        REQUIRE(root.getPath() == ev->filepath);
        root.goUp();
        REQUIRE(root.getPath() == ev->parentpath);
        REQUIRE(ev->action == SpaceWatcher::FileAction::ADDED);
        REQUIRE(watcher->popEvent() == nullptr);

        dh.createFile("test/test.txt");
        //wait for watcher thread to detect events
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        ev = watcher->popEvent();
        REQUIRE(ev != nullptr);
        root.addDir("test");
        root.addFile("test.txt");
        REQUIRE(root.getPath() == ev->filepath);
        root.goUp();
        REQUIRE(root.getPath() == ev->parentpath);
        REQUIRE(ev->action == SpaceWatcher::FileAction::ADDED);
        REQUIRE(watcher->popEvent() == nullptr);
    }

    SECTION("Modify directory or file")
    {
        //create exising dir structure
        dh.createDir("test");
        dh.createFile("test/test.txt");

        REQUIRE_NOTHROW(watcher = SpaceWatcher::create(root.getPath()));
        root.addDir("test");
        watcher->addDir(root.getPath());
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        SECTION("Delete")
        {
            dh.deleteDir("test");

            //wait for watcher thread to detect events
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            auto ev = watcher->popEvent();
            REQUIRE(ev != nullptr);

            root.addFile("test.txt");

            REQUIRE(root.getPath() == ev->filepath);
            root.goUp();
            REQUIRE(root.getPath() == ev->parentpath);
            REQUIRE(ev->action == SpaceWatcher::FileAction::REMOVED);

            ev = watcher->popEvent();

            REQUIRE(ev != nullptr);
            if (ev->filepath.back() != PlatformUtils::filePathSeparator)
                ev->filepath.push_back(PlatformUtils::filePathSeparator);

            REQUIRE(root.getPath() == ev->filepath);
            root.goUp();
            REQUIRE(root.getPath() == ev->parentpath);
            REQUIRE(ev->action == SpaceWatcher::FileAction::REMOVED);
        }

        SECTION("Rename")
        {
            dh.rename("test/test.txt", "test/renamed_test.txt");

            //wait for watcher thread to detect events
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            auto ev = watcher->popEvent();
            REQUIRE(ev != nullptr);

            root.addFile("test.txt");

            REQUIRE(root.getPath() == ev->filepath);
            root.goUp();
            REQUIRE(root.getPath() == ev->parentpath);
            REQUIRE(ev->action == SpaceWatcher::FileAction::OLD_NAME);

            ev = watcher->popEvent();

            REQUIRE(ev != nullptr);

            root.addFile("renamed_test.txt");
            REQUIRE(root.getPath() == ev->filepath);
            root.goUp();
            REQUIRE(root.getPath() == ev->parentpath);
            REQUIRE(ev->action == SpaceWatcher::FileAction::NEW_NAME);
        }

        SECTION("Modify")
        {
            root.addFile("test.txt");
            std::ofstream fs(root.getPath());
            fs << "test\n";
            fs.close();

            //wait for watcher thread to detect events
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            auto ev = watcher->popEvent();
            REQUIRE(ev != nullptr);

            REQUIRE(root.getPath() == ev->filepath);
            root.goUp();
            REQUIRE(root.getPath() == ev->parentpath);
            REQUIRE(ev->action == SpaceWatcher::FileAction::MODIFIED);
        }
        REQUIRE(watcher->popEvent() == nullptr);
    }
}

