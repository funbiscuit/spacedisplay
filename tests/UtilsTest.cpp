
#include <catch.hpp>

#include "utils.h"


TEST_CASE( "Utility tests", "[utils]" )
{
    std::vector<int> testArr = {1,2,40,23,22,22};

    REQUIRE( Utils::in_array(40, testArr) );
    REQUIRE( Utils::in_array(22, testArr) );
    REQUIRE_FALSE( Utils::in_array(30, testArr) );

    REQUIRE( Utils::clip(10, 10, 20) == 10 );
    REQUIRE( Utils::clip(20, 10, 20) == 20 );
    REQUIRE( Utils::clip(15, 10, 20) == 15 );
    REQUIRE( Utils::clip(5, 10, 20) == 10 );
    REQUIRE( Utils::clip(25, 10, 20) == 20 );

    int64_t size = 54;
    REQUIRE( Utils::formatSize(size) == "54.0 B" );
    size = 13;
    REQUIRE( Utils::formatSize(size*1024) == "13.0 KiB" );
    size = 25;
    REQUIRE( Utils::formatSize(size*1024*1024) == "25.0 MiB" );
    size = 90;
    REQUIRE( Utils::formatSize(size*1024*1024*1024) == "90.0 GiB" );

    REQUIRE(Utils::roundFrac(10, 2) == 5 );
    REQUIRE(Utils::roundFrac(11, 2) == 6 );
    REQUIRE(Utils::roundFrac(10, 3) == 3 );
    REQUIRE(Utils::roundFrac(11, 3) == 4 );
    REQUIRE(Utils::roundFrac(50, 7) == 7 );
    REQUIRE(Utils::roundFrac(51, 7) == 7 );
    REQUIRE(Utils::roundFrac(52, 7) == 7 );
    REQUIRE(Utils::roundFrac(53, 7) == 8 );
    REQUIRE(Utils::roundFrac(54, 7) == 8 );
    REQUIRE(Utils::roundFrac(55, 7) == 8 );

}
