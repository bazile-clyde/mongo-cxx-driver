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

#include <fstream>
#include <regex>

#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/test_util/catch.hh>
#include <bsoncxx/types/bson_value/value.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/test/spec/test_runner.hh>
#include <mongocxx/test/spec/util.hh>
#include <mongocxx/test_util/client_helpers.hh>

namespace {

using namespace mongocxx;
using namespace spec;

constexpr int runner_version[3] = {1, 0, 0};

void _run_unified_format_tests_in_file(std::string test_path) {
    using bsoncxx::types::bson_value::value;
    using bsoncxx::v_noabi::stdx::optional;

    // parse test file #############################################################################
    optional<document::value> test_spec = test_util::parse_test_file(test_path);
    REQUIRE(test_spec);
    auto test_spec_view = test_spec->view();

    // schemaVersion. required #####################################################################
    const std::string sv = test_spec_view["schemaVersion"].get_string().value.to_string();
    REQUIRE(sv.size());

    std::vector<int> schema_version;

    const std::regex period("\\.");
    std::transform(std::sregex_token_iterator(begin(sv), end(sv), period, -1),
                   std::sregex_token_iterator(),
                   std::back_inserter(schema_version),
                   [](const std::string s) { return std::stoi(s); });

    enum v { k_major, k_minor, k_patch };
    bool compatible = schema_version[v::k_major] == runner_version[v::k_major] &&
                      schema_version[v::k_minor] <= runner_version[v::k_minor];
    if (!compatible)
        return;

    // runOnRequirements. optional #################################################################
    if (test_spec_view["runOnRequirements"]) {
        array::view run_on = test_spec_view["runOnRequirements"].get_array().value;
        for (auto requirement : run_on) {
            std::cout << "WE HAVE RUN ON REQS!" << std::endl;
            std::cout << bsoncxx::to_json(requirement.get_document()) << std::endl;
            if (auto min_ser_ver = requirement["minServerVersion"])
                std::cout << min_ser_ver.get_string().value << std::endl;
            if (auto max_ser_ver = requirement["maxServerVersion"])
                std::cout << max_ser_ver.get_string().value << std::endl;
            if (auto topo = requirement["topologies"])
                std::cout << bsoncxx::to_json(topo.get_array().value) << std::endl;
        }
    }

    // description. required #######################################################################
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
}  // namespace
