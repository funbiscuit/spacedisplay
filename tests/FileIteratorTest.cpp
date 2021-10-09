#include "filepath.h"
#include "FileIterator.h"
#include "DirHelper.h"

#include <sstream>

struct File {
    std::string name;
    bool isDir;
    int64_t size;

    explicit File(const FileIterator &it) :
            File(it.getName(), it.isDir(), it.getSize()) {}

    explicit File(std::string name, bool isDir = false, int64_t size = 0) : name(std::move(name)), isDir(isDir),
                                                                            size(size) {}

    bool operator==(const File &other) const {
        return isDir == other.isDir && name == other.name;
    };
};

std::ostream &operator<<(std::ostream &os, File const &value) {
    if (value.isDir)
        os << "{ *dir* ";
    else
        os << "{ *file* ";
    os << value.name << " }";
    return os;
}

#include <catch.hpp>

TEST_CASE("FileIterator tests", "[file]")
{
    DirHelper dh("TestDir");
    SECTION("Iterator of non existing dir")
    {
        dh.deleteDir("dir");
        auto it = FileIterator::create("dir");
        REQUIRE_FALSE(it->isValid());
        REQUIRE_THROWS_AS(it->getName(), std::out_of_range);
        REQUIRE_THROWS_AS(it->isDir(), std::out_of_range);
        REQUIRE_THROWS_AS(it->getSize(), std::out_of_range);
        REQUIRE_THROWS_AS(it->operator++(), std::out_of_range);
    }

    SECTION("Iterator of existing dir")
    {
        std::vector<File> expectedFiles{
                File("dir", true),
                File("file.txt"),
                File("file2.txt"),
                File("5.txt"),
                File("dir2", true),
        };

        for (auto &f : expectedFiles) {
            if (f.isDir)
                dh.createDir(f.name);
            else
                dh.createFile(f.name);
        }
        dh.createFile("dir/3.txt");
        dh.createFile("dir/4.txt");

        auto it = FileIterator::create("TestDir");
        REQUIRE(it->isValid());

        std::vector<File> files;
        //order is not specified, so need to collect all and then check
        while (it->isValid()) {
            files.emplace_back(*it);
            ++(*it);
        }

        REQUIRE_THAT(files, Catch::Matchers::UnorderedEquals(expectedFiles));
    }
}
