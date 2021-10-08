#include <catch.hpp>
#include <iostream>

#include "fileentry.h"
#include "utils.h"


TEST_CASE("FileEntry construction", "[fileentry]")
{
    SECTION("From empty name")
    {
        REQUIRE_THROWS(FileEntry("", false, 150));
    }SECTION("From non-empty name")
    {
        std::string entryName = "TestEntry";
        int entrySize = 150;
        FileEntry entry(entryName, true, entrySize);

        REQUIRE(entryName == entry.getName());
        entryName = "SomeOther";
        REQUIRE(entryName != entry.getName());
        REQUIRE(entrySize == entry.getSize());
        REQUIRE(entry.isDir());
        REQUIRE(entry.isRoot());
        REQUIRE(entry.getParent() == nullptr);
        REQUIRE(entry.getNameCrc() == entry.getPathCrc());
    }
}

TEST_CASE("FileEntry simple tree construction", "[fileentry]")
{
    FileEntry root("/root/", true);


    INFO("Add files");

    int totalSize = 0;
    FileEntry *lastChild;
    for (int i = 0; i < 100; ++i) {
        auto childName = Utils::strFormat("Child%d", i);
        auto child = Utils::make_unique<FileEntry>(childName, false, i * 10);
        totalSize += child->getSize();
        lastChild = child.get();
        root.addChild(std::move(child));
        REQUIRE(lastChild->getParent() == &root);
        REQUIRE_FALSE(lastChild->isRoot());
    }

    REQUIRE(root.getSize() == totalSize);

    INFO("Check that children are sorted");
    const FileEntry *prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });

    INFO("Add new children");
    for (int i = 0; i < 200; ++i) {
        auto childName = Utils::strFormat("Child%d", i + 100);
        auto child = Utils::make_unique<FileEntry>(childName, false, i * 5);
        totalSize += child->getSize();
        lastChild = child.get();
        root.addChild(std::move(child));
        REQUIRE(lastChild->getParent() == &root);
        REQUIRE_FALSE(lastChild->isRoot());
    }
    REQUIRE(root.getSize() == totalSize);

    INFO("Check that children are still sorted");
    prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });

    INFO("Check that children are sorted after changing size of child");
    lastChild->setSize(25);
    prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });


    SECTION("Remove with mark for delete")
    {
        SECTION("Remove all children")
        {
            int files, dirs;
            root.markChildrenPendingDelete(files, dirs);
            REQUIRE(files == 300);
            REQUIRE(dirs == 0);
            std::vector<std::unique_ptr<FileEntry>> deleted;
            root.removePendingDelete(deleted);
            REQUIRE(deleted.size() == files);
            REQUIRE(root.getSize() == 0);
        }SECTION("Remove half of children")
        {
            int files, dirs;
            root.markChildrenPendingDelete(files, dirs);

            int counter = 0;
            auto newSize = root.getSize();
            root.forEach([&counter, &files, &newSize](const FileEntry &child) -> bool {
                if (counter % 2 == 0)
                    ((FileEntry &) child).unmarkPendingDelete();
                else {
                    --files;
                    newSize -= child.getSize();
                }
                ++counter;
                return true;
            });
            REQUIRE(files == 150);
            std::vector<std::unique_ptr<FileEntry>> deleted;
            root.removePendingDelete(deleted);
            REQUIRE(deleted.size() == 150);
            REQUIRE(root.getSize() == newSize);
        }SECTION("Remove one third of children")
        {
            int files, dirs;
            root.markChildrenPendingDelete(files, dirs);

            int counter = 0;
            auto newSize = root.getSize();
            root.forEach([&counter, &files, &newSize](const FileEntry &child) -> bool {
                if (counter % 3 != 0)
                    ((FileEntry &) child).unmarkPendingDelete();
                else {
                    --files;
                    newSize -= child.getSize();
                }
                ++counter;
                return true;
            });
            REQUIRE(files == 200);
            std::vector<std::unique_ptr<FileEntry>> deleted;
            root.removePendingDelete(deleted);
            REQUIRE(deleted.size() == 100);
            REQUIRE(root.getSize() == newSize);
        }

        INFO("Check that children are sorted after deletion");
        prevEntry = nullptr;
        root.forEach([&prevEntry](const FileEntry &child) -> bool {
            if (prevEntry)
                REQUIRE(child.getSize() <= prevEntry->getSize());
            else
                prevEntry = &child;
            return true;
        });
    }
}


TEST_CASE("FileEntry deep tree construction", "[fileentry]")
{
    FileEntry root("/root/", true);

    INFO("Add files");

    int totalSize = 0;
    FileEntry *lastParent = &root;
    std::vector<FileEntry *> children;
    for (int i = 0; i < 5; ++i) {
        auto childName = Utils::strFormat("Child_%d", i);
        auto child = Utils::make_unique<FileEntry>(childName, true);
        totalSize += child->getSize();
        auto lastChild = child.get();
        root.addChild(std::move(child));
        REQUIRE(lastChild->getParent() == lastParent);
        REQUIRE_FALSE(lastChild->isRoot());
        children.push_back(lastChild);
    }

    REQUIRE(root.getSize() == totalSize);
    std::random_device r;
    std::default_random_engine randEngine(r());
    std::uniform_int_distribution<int> intDistr(1, 10);
    for (auto &child : children) {
        for (int i = 0; i < 5; ++i) {
            auto childName = Utils::strFormat("Child_%d", i);
            auto newChild = Utils::make_unique<FileEntry>(childName, true, intDistr(randEngine));
            totalSize += newChild->getSize();
            auto lastChild = newChild.get();
            child->addChild(std::move(newChild));
            REQUIRE(lastChild->getParent() == child);
            REQUIRE_FALSE(lastChild->isRoot());
        }
    }
    REQUIRE(root.getSize() == totalSize);

    INFO("Check that children are sorted");
    const FileEntry *prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });

    children.clear();
    root.forEach([&children](const FileEntry &entry) -> bool {
        children.push_back((FileEntry *) &entry);
        return true;
    });
    //reverse initial order
    for (int i = 0; i < children.size(); ++i)
        children[i]->setSize(children[i]->getSize() + 10 * i);

    INFO("Check that children are sorted");
    prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });
    //restore order
    for (int i = 0; i < children.size(); ++i)
        children[i]->setSize(children[i]->getSize() - 10 * i);
    INFO("Check that children are sorted");
    prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });

    INFO("Remove all children in first subdir");
    FileEntry *subdir = nullptr;
    root.forEach([&subdir](const FileEntry &child) -> bool {
        subdir = (FileEntry *) &child;
        return false;
    });
    int files, dirs;
    subdir->markChildrenPendingDelete(files, dirs);
    REQUIRE(dirs > 0);
    REQUIRE(subdir->getSize() > 0);
    std::vector<std::unique_ptr<FileEntry>> deleted;
    subdir->removePendingDelete(deleted);
    REQUIRE(dirs == deleted.size());
    REQUIRE(subdir->getSize() == 0);

    INFO("Check that children are sorted");
    prevEntry = nullptr;
    root.forEach([&prevEntry](const FileEntry &child) -> bool {
        if (prevEntry)
            REQUIRE(child.getSize() <= prevEntry->getSize());
        else
            prevEntry = &child;
        return true;
    });
}
