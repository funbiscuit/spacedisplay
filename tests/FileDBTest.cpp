#include "filedb.h"
#include "filepath.h"
#include "fileentry.h"
#include "utils.h"

#include <cstring>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("FileDB construction", "[filedb]")
{
    FileDB db;

    REQUIRE_FALSE(db.isReady());
    REQUIRE(db.getRootPath() == nullptr);
    REQUIRE(db.getDirCount() == 0);
    REQUIRE(db.getFileCount() == 0);


    db.setNewRootPath("/home/");
    REQUIRE(db.isReady());
    REQUIRE(db.getRootPath() != nullptr);
    FilePath rootPath("/home");
    REQUIRE(db.getRootPath()->getPath() == rootPath.getPath());
    REQUIRE(db.getDirCount() == 1);
    REQUIRE(db.getFileCount() == 0);
    REQUIRE(db.hasChanges());

    db.clearDb();
    REQUIRE_FALSE(db.isReady());
    REQUIRE(db.getRootPath() == nullptr);
    REQUIRE(db.getDirCount() == 0);
    REQUIRE(db.getFileCount() == 0);
}


void createSampleDb(FileDB &db) {
    FilePath path("/home");
    std::vector<std::unique_ptr<FileEntry>> entries;
    db.clearDb();

    entries.push_back(Utils::make_unique<FileEntry>("dir1", true));
    REQUIRE_FALSE(db.setChildrenForPath(path, std::move(entries))); //add to not initialized db

    db.setNewRootPath("/home");

    entries.push_back(Utils::make_unique<FileEntry>("dir1", true));
    entries.push_back(Utils::make_unique<FileEntry>("dir2", true));
    entries.push_back(Utils::make_unique<FileEntry>("dir3", true));

    std::vector<std::unique_ptr<FilePath>> newPaths;
    REQUIRE(db.setChildrenForPath(path, std::move(entries), &newPaths));
    REQUIRE(newPaths.size() == 3);
    REQUIRE(newPaths[0]->getName() == "dir1");

    path.addDir("dir12");
    entries.push_back(Utils::make_unique<FileEntry>("file1", false, 10));
    REQUIRE_FALSE(db.setChildrenForPath(path, std::move(entries))); //add to non existent path

    path.goUp();
    path.addFile("dir1");
    entries.push_back(Utils::make_unique<FileEntry>("file1", false, 10));
    REQUIRE_FALSE(db.setChildrenForPath(path, std::move(entries))); //add to file path

    path.goUp();
    path.addDir("dir1");
    REQUIRE_FALSE(db.setChildrenForPath(path, std::move(entries))); //add empty array


    entries.push_back(Utils::make_unique<FileEntry>("file1", false, 10));
    entries.push_back(Utils::make_unique<FileEntry>("file2", false, 30));
    entries.push_back(Utils::make_unique<FileEntry>("file3", false, 20));

    REQUIRE(db.setChildrenForPath(path, std::move(entries)));

    path.goUp();
    path.addDir("dir2");
    entries.push_back(Utils::make_unique<FileEntry>("file4", false, 15));
    entries.push_back(Utils::make_unique<FileEntry>("file5", false, 35));
    entries.push_back(Utils::make_unique<FileEntry>("file6", false, 25));

    REQUIRE(db.setChildrenForPath(path, std::move(entries)));

    path.goUp();
    path.addDir("dir3");
    entries.push_back(Utils::make_unique<FileEntry>("file7", false, 15));
    entries.push_back(Utils::make_unique<FileEntry>("file8", false, 35));
    entries.push_back(Utils::make_unique<FileEntry>("file9", false, 25));

    REQUIRE(db.setChildrenForPath(path, std::move(entries)));

    db.setSpace(500, 100);
    uint64_t used, available, total;
    db.getSpace(used, available, total);
    REQUIRE(total == 500);
    REQUIRE(available == 100);
    REQUIRE(used == 210);

    REQUIRE(db.getFileCount() == 9);
    REQUIRE(db.getDirCount() == 4);
}

TEST_CASE("FileDB searching", "[filedb]")
{
    FileDB db;
    FilePath path("/home/");

    auto entry = db.findEntry(path);
    REQUIRE(entry == nullptr);
    createSampleDb(db);

    entry = db.findEntry(path);
    REQUIRE(entry != nullptr);
    REQUIRE(path.getPath() == entry->getName());

    entry = db.findEntry(FilePath("/home2"));
    REQUIRE(entry == nullptr);

    path.addDir("dir1");
    path.addFile("file1");
    entry = db.findEntry(path);
    REQUIRE(entry != nullptr);

    path.goUp();
    path.goUp();
    path.addDir("dir2");
    path.addFile("file4");
    entry = db.findEntry(path);
    REQUIRE(entry != nullptr);

    path.goUp();
    path.goUp();
    path.addDir("dir3");
    path.addFile("file7");
    entry = db.findEntry(path);
    REQUIRE(entry != nullptr);

    db.clearDb();
    auto processed = db.processEntry(path, [](const FileEntry &entry) {
        REQUIRE(false); // not executed
    });
    REQUIRE_FALSE(processed);

    createSampleDb(db);

    path.goUp();
    path.goUp();
    path.addDir("dir1");
    path.addFile("file1");

    processed = db.processEntry(path, [](const FileEntry &entry) {
        REQUIRE(strcmp(entry.getName(), "file1") == 0);
    });
    REQUIRE(processed);
    path.goUp();
    path.addFile("file4");
    processed = db.processEntry(path, [](const FileEntry &entry) {
        REQUIRE(false); // not executed
    });
    REQUIRE_FALSE(processed);

    path.goUp();
}

TEST_CASE("FileDB modification", "[filedb]")
{
    FileDB db;
    FilePath path("/home/");
    createSampleDb(db);

    path.addDir("dir1");

    SECTION("Update children")
    {
        SECTION("Update files")
        {
            path.addFile("file1");
            auto oldEntry = db.findEntry(path);
            REQUIRE(oldEntry != nullptr);
            path.goUp();

            std::vector<std::unique_ptr<FileEntry>> entries;
            entries.push_back(Utils::make_unique<FileEntry>("file2", false, 128));
            entries.push_back(Utils::make_unique<FileEntry>("file3", false, 20));
            entries.push_back(Utils::make_unique<FileEntry>("file5", false, 64));

            REQUIRE(db.setChildrenForPath(path, std::move(entries)));
            REQUIRE(db.getFileCount() == 9);
            REQUIRE(db.getDirCount() == 4);
            auto dir1 = db.findEntry(path);
            REQUIRE(dir1 != nullptr);
            std::vector<std::string> names;
            dir1->forEach([&names](const FileEntry &entry) -> bool {
                names.emplace_back(entry.getName());
                return true;
            });
            REQUIRE(names.size() == 3);
            REQUIRE(names[0] == "file2");
            REQUIRE(names[1] == "file5");
            REQUIRE(names[2] == "file3");
            path.addFile("file1");
            oldEntry = db.findEntry(path);
            REQUIRE(oldEntry == nullptr);
        }

        SECTION("Update dirs")
        {
            path.goUp();//dir1
            path.goUp();//root
            auto oldDir = db.findEntry(path);
            REQUIRE(oldDir != nullptr);

            std::vector<std::unique_ptr<FileEntry>> entries;
            entries.push_back(Utils::make_unique<FileEntry>("dir2", true));
            entries.push_back(Utils::make_unique<FileEntry>("dir5", true, 64));

            REQUIRE(db.setChildrenForPath(path, std::move(entries)));
            REQUIRE(db.getFileCount() == 3);
            REQUIRE(db.getDirCount() == 3);
            auto rootEntry = db.findEntry(path);
            REQUIRE(rootEntry != nullptr);
            std::vector<std::string> names;
            rootEntry->forEach([&names](const FileEntry &entry) -> bool {
                names.emplace_back(entry.getName());
                return true;
            });
            REQUIRE(names.size() == 2);
            REQUIRE(names[0] == "dir2");
            REQUIRE(names[1] == "dir5");
            path.addDir("dir1");
            auto oldEntry = db.findEntry(path);
            REQUIRE(oldEntry == nullptr);

            path.addFile("file1");
            oldEntry = db.findEntry(path);
            REQUIRE(oldEntry == nullptr);

            path.goUp();
            path.addFile("file2");
            oldEntry = db.findEntry(path);
            REQUIRE(oldEntry == nullptr);

            path.goUp();
            path.goUp();
            path.addDir("dir3");
            oldEntry = db.findEntry(path);
            REQUIRE(oldEntry == nullptr);

            path.addFile("file7");
            oldEntry = db.findEntry(path);
            REQUIRE(oldEntry == nullptr);
        }
    }
}
