
#include <catch.hpp>

#include "spacescanner.h"
#include "filepath.h"
#include "filedb.h"
#include "utils.h"
#include "DirHelper.h"

#include <iostream>

TEST_CASE("Scanner tests", "[scanner]")
{
    auto scanner = Utils::make_unique<SpaceScanner>();

    REQUIRE_FALSE(scanner->is_loaded());

    std::unique_ptr<FilePath> curScanPath;

    scanner->getCurrentScanPath(curScanPath);

    REQUIRE(curScanPath == nullptr);

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

        REQUIRE(scanner->scan_dir("TestDir2") == ScannerError::CANT_OPEN_DIR);
        REQUIRE(scanner->scan_dir("TestDir") == ScannerError::NONE);
        REQUIRE(scanner->scan_dir("TestDir") == ScannerError::SCAN_RUNNING);

        REQUIRE(scanner->is_loaded());
        REQUIRE(scanner->getRootPath() != nullptr);
        REQUIRE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE_FALSE(scanner->isProgressKnown());
        REQUIRE(scanner->has_changes());
        REQUIRE(scanner->can_refresh());

        //wait for scanner to complete
        while (scanner->get_scan_progress() < 100)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE_FALSE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE(scanner->has_changes());
        REQUIRE(scanner->can_refresh());

        REQUIRE(scanner->getDirCount() == 4);
        REQUIRE(scanner->getFileCount() == 9);
        scanner->rescan_dir(FilePath("TestDir"));
        REQUIRE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());


        //wait for scanner to complete
        while (scanner->get_scan_progress() < 100)
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
        REQUIRE(scanner->get_scan_progress() == 100);

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
        auto roots = scanner->get_available_roots();
        REQUIRE_FALSE(roots.empty());
        ScannerError err = scanner->scan_dir(roots.front());
        if (err == ScannerError::CANT_OPEN_DIR) {
            std::cout << "Root dir is not available, can't test root scanning!\n";
            return;
        }

        REQUIRE(err == ScannerError::NONE);
        REQUIRE(scanner->is_loaded());
        REQUIRE(scanner->getRootPath() != nullptr);
        REQUIRE(scanner->canPause());
        REQUIRE_FALSE(scanner->canResume());
        REQUIRE(scanner->isProgressKnown());
        REQUIRE(scanner->has_changes());
        REQUIRE(scanner->can_refresh());

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
            scanner->getCurrentScanPath(curScanPath);
            REQUIRE(curScanPath != nullptr);
            scanner->getCurrentScanPath(curScanPath);
            REQUIRE(curScanPath != nullptr);

            REQUIRE(scanner->has_changes());
            auto db = scanner->getFileDB();
            REQUIRE(db != nullptr);
            auto root = scanner->getRootPath();
            REQUIRE(root != nullptr);
            auto processed = db->processEntry(*root, [](const FileEntry &entry) {});
            REQUIRE(processed);
            REQUIRE_FALSE(scanner->has_changes());

            REQUIRE(scanner->getDirCount() > 0);
            REQUIRE(scanner->getFileCount() > 0);

            REQUIRE(scanner->resumeScan());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            REQUIRE(scanner->canPause());
            REQUIRE_FALSE(scanner->canResume());
            scanner->stop_scan();
            REQUIRE_FALSE(scanner->canPause());
            REQUIRE_FALSE(scanner->canResume());
            REQUIRE(scanner->get_scan_progress() == 100);

            uint64_t used, available, total;
            scanner->getSpace(used, available, total);
            REQUIRE(used > 0);
            REQUIRE(available > 0);
            REQUIRE(total > 0);
            REQUIRE((total - available) >= used);
        }
    }
}
