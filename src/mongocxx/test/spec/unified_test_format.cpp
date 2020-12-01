// Copyright 2020 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/test_util/catch.hh>
#include <fstream>
#include <mongocxx/instance.hpp>
#include <mongocxx/test/spec/test_runner.hh>
#include <mongocxx/test/spec/util.hh>
#include <mongocxx/test_util/client_helpers.hh>

namespace {

using namespace mongocxx;
using namespace spec;

void _run_unified_format_tests_in_file(std::string test_path) {
    using bsoncxx::v_noabi::stdx::optional;

    // parse test file #############################################################################
    INFO("Test path: " << test_path);
    optional<document::value> test_spec = test_util::parse_test_file(test_path);
    REQUIRE(test_spec);

    std::cout << bsoncxx::to_json(test_spec->view()) << std::endl;
    // ############################################################################################
    auto test_spec_view = test_spec->view();
    std::string description = test_spec_view["description"].get_string().value.to_string();
    SECTION(description) {}
}

TEST_CASE("unified format spec automated tests", "[unified_format_spec]") {
    instance::current();

    std::string path = std::getenv("UNIFIED_FORMAT_TESTS_PATH");
    REQUIRE(path.size());

    std::ifstream test_files{path + "/test_files.txt"};
    REQUIRE(test_files.good());

    std::string test_file;
    while (std::getline(test_files, test_file)) {
        _run_unified_format_tests_in_file(path + "/" + test_file);
    }
}
}
