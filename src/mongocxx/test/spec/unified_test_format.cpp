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
enum v { k_major, k_minor, k_patch };

std::vector<int> parse_version(const std::string& input) {
    std::vector<int> output;
    const std::regex period("\\.");
    std::transform(std::sregex_token_iterator(std::begin(input), std::end(input), period, -1),
                   std::sregex_token_iterator(),
                   std::back_inserter(output),
                   [](const std::string s) { return std::stoi(s); });
    return output;
}

std::vector<int> parse_version(document::element doc) {
    return parse_version(doc.get_string().value.to_string());
}

template <typename Compare>
bool compare_to_server_version(const std::vector<int>& version, Compare cmp) {
    static std::vector<int> server_version = parse_version(test_util::get_server_version());
    return std::lexicographical_compare(std::begin(version),
                                        std::next(std::begin(version), v::k_patch),
                                        std::begin(server_version),
                                        std::next(std::begin(server_version), v::k_patch),
                                        cmp);
}

bool compare_to_server_topology(array::view topologies) {
    static std::string server_topology = test_util::get_topology();

    auto equals = [&](const array::element& element) {
        auto topology = element.get_string().value.to_string();
        return topology == server_topology ||
               (topology == "sharded-replicaset" && server_topology == "shared");
    };

    return std::end(topologies) !=
           std::find_if(std::begin(topologies), std::end(topologies), equals);
}

bool compatible_with_server(const array::element& requirement) {
    if (auto min_server_version = requirement["minServerVersion"])
        if (!compare_to_server_version(parse_version(min_server_version), std::less_equal<int>{}))
            return false;

    if (auto max_server_version = requirement["maxServerVersion"])
        if (!compare_to_server_version(parse_version(max_server_version),
                                       std::greater_equal<int>{}))
            return false;

    if (auto topologies = requirement["topologies"])
        return compare_to_server_topology(topologies.get_array().value);
    return true;
}

bool run_on_requirements(const document::view test) {
    if (!test["runOnRequirements"])
        return true; /* optional */

    auto requirements = test["runOnRequirements"].get_array().value;
    return std::any_of(std::begin(requirements), std::end(requirements), compatible_with_server);
}

void _run_unified_format_tests_in_file(std::string test_path) {
    using bsoncxx::types::bson_value::value;
    using bsoncxx::v_noabi::stdx::optional;

    // parse test file #############################################################################
    optional<document::value> test_spec = test_util::parse_test_file(test_path);
    REQUIRE(test_spec);
    auto test_spec_view = test_spec->view();

    // schemaVersion. required #####################################################################
    REQUIRE(test_spec_view["schemaVersion"]);
    std::vector<int> schema_version =
        parse_version(test_spec_view["schemaVersion"].get_string().value.to_string());

    bool compatible = schema_version[v::k_major] == runner_version[v::k_major] &&
                      schema_version[v::k_minor] <= runner_version[v::k_minor];
    if (!compatible)
        return;

    // runOnRequirements. optional #################################################################
    if (!run_on_requirements(test_spec_view))
        return;

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
