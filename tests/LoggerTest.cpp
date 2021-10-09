#include "logger.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Logger tests", "[logger]")
{
    Logger logger;

    auto history = logger.getHistory();

    REQUIRE(history.empty());
    REQUIRE_FALSE(logger.hasNew());

    SECTION("Add logs with default tag")
    {
        logger.log("test entry");
        logger.log("new entry");
        REQUIRE(logger.hasNew());
        history = logger.getHistory();
        REQUIRE(history.size() == 2);
        REQUIRE_FALSE(logger.hasNew());

        history[0] = "[LOG] test entry";
        history[1] = "[LOG] new entry";
    }

    SECTION("Add logs with custom tag")
    {
        logger.log("test entry", "SCAN");
        logger.log("new entry", "WATCH");
        REQUIRE(logger.hasNew());
        history = logger.getHistory();
        REQUIRE(history.size() == 2);
        REQUIRE_FALSE(logger.hasNew());

        history[0] = "[SCAN] test entry";
        history[1] = "[WATCH] new entry";
    }
}
