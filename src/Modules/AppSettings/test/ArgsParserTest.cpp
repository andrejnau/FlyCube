#include "AppSettings/ArgsParser.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>

TEST_CASE("--gpu requires an index")
{
    char executable[] = "FlyCube";
    char option[] = "--gpu";
    char* argv[] = { executable, option, nullptr };

    REQUIRE_THROWS_AS(ParseArgs(2, argv), std::invalid_argument);
}

TEST_CASE("--gpu selects the requested adapter index")
{
    char executable[] = "FlyCube";
    char option[] = "--gpu";
    char index[] = "3";
    char* argv[] = { executable, option, index, nullptr };

    REQUIRE(ParseArgs(3, argv).required_gpu_index == 3);
}
