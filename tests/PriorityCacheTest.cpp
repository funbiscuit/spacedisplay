#include "PriorityCache.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Priority cache tests with no supplier", "[priority-cache]")
{
    PriorityCache<int, int> cache;

    SECTION("Get throws when accessing non cached values")
    {
        REQUIRE_THROWS_AS(cache.get(1), std::runtime_error);
    }

    SECTION("Get does not throw when accessing cached values")
    {
        cache.put(1, 5);
        REQUIRE_NOTHROW(cache.get(1));
        REQUIRE(cache.get(1) == 5);
    }

    SECTION("Is cached works for cached and not cached keys") {
        REQUIRE_FALSE(cache.isCached(1));
        cache.put(1, 1);
        REQUIRE(cache.isCached(1));
    }
}

TEST_CASE("Priority cache tests", "[priority-cache]")
{
    size_t callCount = 0;
    auto supplier = [&callCount](int key) -> int {
        ++callCount;
        return key * key;
    };

    PriorityCache<int, int> cache(supplier);

    SECTION("Get returns correct results")
    {
        for (int i = -10; i < 10; ++i) {
            REQUIRE(cache.get(i) == i * i);
        }
    }

    SECTION("Get calls supplier only once for each unique key")
    {
        for (int i = 0; i < 10; ++i) {
            cache.get(1);
        }
        REQUIRE(callCount == 1);
        for (int i = 0; i < 10; ++i) {
            cache.get(i);
        }
        REQUIRE(callCount == 10);
    }

    SECTION("Trim removes oldest values from cache")
    {
        for (int i = 0; i < 10; ++i) {
            cache.get(i);
        }
        cache.trim(5);
        for (int i = 0; i < 10; ++i) {
            if (i < 5) {
                REQUIRE_FALSE(cache.isCached(i));
            } else {
                REQUIRE(cache.isCached(i));
            }
        }
    }

    SECTION("Access time of key is checked when trimming")
    {
        for (int i = 0; i < 10; ++i) {
            cache.get(i);
        }
        cache.get(0);
        cache.trim(5);
        for (int i = 0; i < 10; ++i) {
            if (i < 6 && i > 0) {
                REQUIRE_FALSE(cache.isCached(i));
            } else {
                REQUIRE(cache.isCached(i));
            }
        }
    }

    SECTION("Accessing key multiple times doesn't make it newer")
    {
        // even if key was accessed a lot of times, it will be dropped after
        // trim if there are newer keys
        for (int i = 0; i < 100; ++i) {
            cache.get(0);
        }
        for (int i = 1; i < 6; ++i) {
            cache.get(i);
        }
        cache.trim(5);
        REQUIRE_FALSE(cache.isCached(0));
        for (int i = 1; i < 6; ++i) {
            REQUIRE(cache.isCached(i));
        }
    }

    SECTION("With max size set, values are automatically removed") {
        int maxSize = 5;
        cache.setMaxSize(maxSize);
        for (int i = 0; i < maxSize; ++i) {
            cache.get(i);
        }
        for (int i = 0; i < maxSize; ++i) {
            REQUIRE(cache.isCached(i));
        }
        for (int i = maxSize; i < 2*maxSize; ++i) {
            cache.get(i);

            for (int j = 0; j <= i; ++j) {
                INFO("Check key " << j << " when added key " << i);
                if (i - j >= maxSize) {
                    REQUIRE_FALSE(cache.isCached(j));
                } else {
                    REQUIRE(cache.isCached(j));
                }
            }
        }
    }

    SECTION("Set max size trims if needed")
    {
        int maxSize = 5;
        for (int i = 0; i < 2*maxSize; ++i) {
            cache.get(i);
        }
        cache.setMaxSize(maxSize);

        for (int i = 0; i < 2*maxSize; ++i) {
            if (i < maxSize) {
                REQUIRE_FALSE(cache.isCached(i));
            } else {
                REQUIRE(cache.isCached(i));
            }
        }
    }

    SECTION("Trim doesn't change max size")
    {
        int maxSize = 5;
        cache.setMaxSize(maxSize);
        for (int i = 0; i < 2*maxSize; ++i) {
            cache.get(i);
        }
        cache.trim(3);
        for (int i = 0; i < 2*maxSize; ++i) {
            cache.get(i);
        }
        for (int i = 0; i < 2*maxSize; ++i) {
            if (i < maxSize) {
                REQUIRE_FALSE(cache.isCached(i));
            } else {
                REQUIRE(cache.isCached(i));
            }
        }
    }

    SECTION("Invalidate removes all cached values")
    {
        int maxSize = 5;
        for (int i = 0; i < maxSize; ++i) {
            cache.get(i);
        }
        cache.invalidate();
        for (int i = 0; i < maxSize; ++i) {
            REQUIRE_FALSE(cache.isCached(i));
        }
    }
}


TEST_CASE("Priority cache with unique pointers", "[priority-cache]")
{
    auto supplier = [](int key) -> std::unique_ptr<int> {
        return std::unique_ptr<int>(new int(key * key));
    };

    PriorityCache<int, std::unique_ptr<int>> cache(supplier);

    SECTION("Get returns correct results")
    {
        for (int i = -10; i < 10; ++i) {
            REQUIRE((*cache.get(i)) == i * i);
        }
    }

    SECTION("Trim removes oldest values from cache")
    {
        for (int i = 0; i < 10; ++i) {
            cache.get(i);
        }
        cache.get(0);
        cache.trim(5);
        for (int i = 0; i < 10; ++i) {
            if (i < 6 && i > 0) {
                REQUIRE_FALSE(cache.isCached(i));
            } else {
                REQUIRE((*cache.get(i)) == i * i);
            }
        }
    }
}
