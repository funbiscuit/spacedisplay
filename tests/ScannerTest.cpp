#include "spacescanner.h"
#include "filepath.h"
#include "filedb.h"
#include "utils.h"
#include "platformutils.h"
#include "DirHelper.h"

#include <iostream>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Scanner tests", "[scanner]")
{
    SECTION("Scan custom path")
    {
        DirHelper dh("TestDir");
        dh.createDir("test");
        dh.createFile("test/test.txt");
        dh.createFile("test/test2.txt");
        dh.createDir("test3");
        dh.createFile("test3/test.txt");
        dh.createFile("test3/test2.txt");
        dh.createFile("test3/test3.txt");
        dh.createFile("test3/test4.txt");
        dh.createDir("test3/test5");
        dh.createFile("test3/test5/test1.txt");
        dh.createFile("test3/test5/test2.txt");
        dh.createFile("test3/test5/test3.txt");

        std::unique_ptr<SpaceScanner> scanner;
        REQUIRE_THROWS_AS(SpaceScanner("TestDir2"), std::runtime_error);
        REQUIRE_NOTHROW(scanner = Utils::make_unique<SpaceScanner>("TestDir"));

        REQUIRE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE_FALSE(scanner->isProgressKnown());
        REQUIRE(scanner->hasChanges());

        //wait for scanner to complete
        while (scanner->getScanProgress() < 100)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE_FALSE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE(scanner->hasChanges());

        REQUIRE(scanner->getDirCount() == 4);
        REQUIRE(scanner->getFileCount() == 9);
        scanner->rescanPath(FilePath("TestDir"));
        REQUIRE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());


        //wait for scanner to complete
        while (scanner->getScanProgress() < 100)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE(scanner->getDirCount() == 4);
        REQUIRE(scanner->getFileCount() == 9);

        dh.createDir("test3/test2");
        dh.createFile("test3/test2/test1.txt");
        dh.createFile("test3/test2/test2.txt");
        dh.createFile("test3/test2/test3.txt");
        dh.createDir("test3/test1");
        dh.createFile("test3/test1/test2.txt");
        dh.createFile("test3/test1/test3.txt");
        //wait for scanner to detect changes
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        REQUIRE(scanner->getDirCount() == 6);
        REQUIRE(scanner->getFileCount() == 14);

        dh.createDir("test3/test10");
        dh.createFile("test3/test10/test2.txt");
        dh.createFile("test3/test10/test3.txt");
        dh.createDir("test3/test10/test2");
        dh.createFile("test3/test10/test2/test1.txt");
        dh.createFile("test3/test10/test2/test2.txt");
        FilePath path("TestDir");
        path.addDir("test3");

        //wait for scanner to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        REQUIRE_FALSE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE(scanner->getScanProgress() == 100);

        REQUIRE(scanner->getDirCount() == 8);
        REQUIRE(scanner->getFileCount() == 18);

        int64_t watchedNow, limit;
        bool exceeded = scanner->getWatcherLimits(watchedNow, limit);
        REQUIRE_FALSE(exceeded);
        if (limit >= 0)
            REQUIRE(watchedNow == scanner->getDirCount());
        else
            REQUIRE(watchedNow == 1);

        dh.deleteDir("test3/test10");
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        scanner->getWatcherLimits(watchedNow, limit);
        if (limit >= 0)
            REQUIRE(watchedNow == scanner->getDirCount());
        else
            REQUIRE(watchedNow == 1);
    }

    SECTION("Scan root")
    {
        auto roots = PlatformUtils::getAvailableMounts();
        REQUIRE_FALSE(roots.empty());
        std::unique_ptr<SpaceScanner> scanner;
        for (auto &root : roots) {
            try {
                scanner = Utils::make_unique<SpaceScanner>(root);
                break;
            } catch (std::runtime_error &e) {
                std::cout << "Can't scan path '" + root + "'!\n";
            }
        }
        if (!scanner) {
            std::cout << "Root dir is not available, can't test root scanning!\n";
            return;
        }

        REQUIRE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE(scanner->isProgressKnown());
        REQUIRE(scanner->hasChanges());

        //wait for scanner to start scanning
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        SECTION("Pause and stop running scan")
        {
            REQUIRE(scanner->canPause());
            REQUIRE_FALSE(scanner->canResume());
            REQUIRE(scanner->pauseScan());
            REQUIRE_FALSE(scanner->canPause());
            REQUIRE(scanner->canResume());

            //wait for scanner to actually pause
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            REQUIRE_FALSE(scanner->canPause());
            REQUIRE(scanner->canResume());
            auto curScanPath = scanner->getCurrentScanPath();
            REQUIRE(curScanPath != nullptr);
            curScanPath = scanner->getCurrentScanPath();
            REQUIRE(curScanPath != nullptr);

            REQUIRE(scanner->hasChanges());
            auto &db = scanner->getFileDB();
            auto root = scanner->getRootPath();
            auto processed = db.processEntry(root, [](const FileEntry &entry) {});
            REQUIRE(processed);
            REQUIRE_FALSE(scanner->hasChanges());

            REQUIRE(scanner->getDirCount() > 0);
            REQUIRE(scanner->getFileCount() > 0);

            REQUIRE(scanner->resumeScan());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            REQUIRE(scanner->canPause());
            REQUIRE_FALSE(scanner->canResume());
            scanner->stopScan();
            REQUIRE_FALSE(scanner->canPause());
            REQUIRE_FALSE(scanner->canResume());
            REQUIRE(scanner->getScanProgress() == 100);

            uint64_t used, available, total;
            scanner->getSpace(used, available, total);
            REQUIRE(used > 0);
            REQUIRE(available > 0);
            REQUIRE(total > 0);
            REQUIRE((total - available) >= used);
        }
    }
}
