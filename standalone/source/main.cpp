#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cmath>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#endif

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>

#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <nlohmann/json.hpp>
#include <dotenv.h>
#include <rapidcsv.h>

using json = nlohmann::json;

const int WAIT_DURATION = 5; // [s]

namespace ghapi {
    namespace member {
        struct payload {
            std::string email;
            std::string role;
            std::vector<int> team_ids;
        };

        void to_json(json& j, const payload& p) {
            j = json{ {"email", p.email}, {"role", p.role}, {"team_ids", p.team_ids} };
        }

        void from_json(const json& j, payload& p) {
            j.at("email").get_to(p.email);
            j.at("role").get_to(p.email);
            j.at("team_ids").get_to(p.team_ids);
        }
    }

    namespace team {
        struct payload {
            std::string org;
            std::string team_slug;
        };

        void to_json(json& j, const payload& p) {
            j = json{ {"org", p.org}, {"team_slug", p.team_slug} };
        }

        void from_json(const json& j, payload& p) {
            j.at("org").get_to(p.org);
            j.at("team_slug").get_to(p.team_slug);
        }
    }
}

json get_team(const std::string& token, const std::string &org_name, const std::string team_slug) {
    ghapi::team::payload p {org_name, team_slug};
    json payload_j = p;
    std::string body = payload_j.dump();
    std::cout << payload_j.dump() << std::endl;

    curlpp::Easy request;

    std::list<std::string> header;
    header.push_back("User-Agent: OrgBulkInvite");
    header.push_back(fmt::format("Authorization: token {}", token));
    header.push_back("Content-Type: application/json");
    header.push_back("Accept: application/vnd.github.v3+json");

    using namespace curlpp::Options;
    request.setOpt(new CustomRequest{"GET"});
    request.setOpt(new Url(fmt::format("https://api.github.com/orgs/{:s}/teams/{:s}", org_name, team_slug)));
    request.setOpt(new HttpHeader(header));
    request.setOpt(new PostFields(body));
    request.setOpt(new PostFieldSize(body.length()));
    request.setOpt(new Verbose(true));

    std::ostringstream response;
    request.setOpt(new WriteStream(&response));

    bool is_sent = false;
    unsigned int attempt_count = 0;
    while (!is_sent) {
        try {
            // NOTE: Check if we can re-perform or clean up is needed.
            request.perform();
            is_sent = true;
            // std::cout << response.str() << std::endl;

            // TODO: Propoerly handle `Retry-After` if one is received.
            // TODO: Check for 4xx or 5xx and handle the errors properly.
            auto response_code = curlpp::infos::ResponseCode::get(request);
            if (response_code == 200) {
                return json::parse(response.str());
            } else if (response_code >= 400 && response_code <= 499) {

            } else if (response_code >= 500 && response_code <= 599) {

            } else {
                fmt::print(std::cout, "get {:d} which we do not know how to handle\n", response_code);
            }
        } catch (curlpp::RuntimeError &e) {
            std::cerr << "runtime error, waiting before try again\n";
            attempt_count += 1;
            sleep(pow(2, attempt_count));
        }
    }

    return payload_j;
}

void send_invitation(const std::string& token, const std::string &org_name, int team_id, const std::string& email_address) {
    // Construct the payload.
    ghapi::member::payload p {email_address, "direct_member", std::vector<int>{team_id} };
    json payload_j = p;
    std::string body = payload_j.dump();
    std::cout << payload_j.dump() << std::endl;

    curlpp::Easy request;

    std::list<std::string> header;
    header.push_back("User-Agent: OrgBulkInvite");
    header.push_back(fmt::format("Authorization: token {}", token));
    header.push_back("Content-Type: application/json");
    header.push_back("Accept: application/vnd.github.v3+json");

    using namespace curlpp::Options;
    request.setOpt(new CustomRequest{"POST"});
    request.setOpt(new Url(fmt::format("https://api.github.com/orgs/{:s}/invitations", org_name)));
    request.setOpt(new HttpHeader(header));
    request.setOpt(new PostFields(body));
    request.setOpt(new PostFieldSize(body.length()));
    request.setOpt(new Verbose(true));

    std::ostringstream response;
    request.setOpt(new WriteStream(&response));

    bool is_sent = false;
    unsigned int attempt_count = 0;
    while (!is_sent) {
        try {
            // NOTE: Check if we can re-perform or clean up is needed.
            request.perform();
            is_sent = true;
            std::cout << response.str() << std::endl;

            // TODO: Propoerly handle `Retry-After` if one is received.
            // TODO: Check for 4xx or 5xx and handle the errors properly.
            auto response_code = curlpp::infos::ResponseCode::get(request);
            if (response_code == 200) {

            } else if (response_code >= 400 && response_code <= 499) {

            } else if (response_code >= 500 && response_code <= 599) {

            } else {
                fmt::print(std::cout, "get {:d} which we do not know how to handle\n", response_code);
            }
        } catch (curlpp::RuntimeError &e) {
            std::cerr << "runtime error, waiting before try again\n";
            attempt_count += 1;
            sleep(pow(2, attempt_count));
        }
    }
}

int main(int argc, char *argv[]) {

    // Read in the .env file.
    dotenv::env.load_dotenv();
    std::string token = dotenv::env["GH_TOKEN"];
    std::string org_name = dotenv::env["ORG_NAME"];
    int team_id = 0;
    std::string team_slug = "";
    bool has_team_id = false;

    try {
        if (dotenv::env["TEAM_ID"] != "" && dotenv::env["TEAM_SLUG"] != "") {
            std::cerr << "both TEAM_ID and TEAM_SLUG are not empty; will be using TEAM_ID\n";
            team_id = std::stoi(dotenv::env["TEAM_ID"], nullptr, 10);
            has_team_id = true;
        } else if (dotenv::env["TEAM_ID"] != "" && dotenv::env["TEAM_SLUG"] == "") {
            // Just use the team_id.
            team_id = std::stoi(dotenv::env["TEAM_ID"], nullptr, 10);
            has_team_id = true;
        } else if (dotenv::env["TEAM_ID"] == "" && dotenv::env["TEAM_SLUG"] != "") {
            // Use the team_slug to get team_id.
            team_slug = dotenv::env["TEAM_SLUG"];
            has_team_id = false;
        } else {
            std::cerr << "require either TEAM_ID or TEAM_SLUG\n";
            return -1;
        }
    } catch (std::invalid_argument const& ex) {
        std::cerr << "invalid text for a team_id\n";
        return -1;
    } catch (std::out_of_range const& ex) {
        std::cerr << "value of team_id is out of range\n";
        return -1;
    }

    std::string input_filename, team_slug_search;
    cxxopts::Options options("BulkInvite", "Send multiple invites to GitHub organization");
    // clang-format off
    options.add_options()
      ("h,help", "Show help")
      ("f,file", "Input file name", cxxopts::value(input_filename)->default_value("email_addresses.txt"))
      ("t,team-id-only", "Only query for team ID using team slug", cxxopts::value<bool>()->default_value("false"))
      ("d,dry-run", "Does not send the actual invitation out", cxxopts::value<bool>()->default_value("false"));
    // clang-formaat on

    auto result = options.parse(argc, argv);

    if (result["help"].as<bool>()) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // Read in the input file.
    std::ifstream email_file;
    std::vector<std::string> email_addresses;

    if (input_filename.substr(input_filename.find_last_of(".") + 1) == "csv") {
        // Assume a CSV users file from ta-tooling.
        rapidcsv::Document doc(input_filename);

        email_addresses = doc.GetColumn<std::string>("emailHandle");
        std::for_each(email_addresses.begin(), email_addresses.end(), [](std::string &handle){ handle = handle + "@ohio.edu"; });
    } else {
        // Assume text file.
        email_file.open(input_filename.c_str());
        if (!email_file.is_open()) {
            fmt::print(std::cerr, "failed to open file '{}''\n", input_filename);
            return -1;
        }

        std::string line;
        while (std::getline(email_file, line)) {
            auto pos = line.rfind("@ohio.edu");
            if (pos == std::string::npos) {
                fmt::print("'{}' is not a valid OHIO email address. it will be skipped.\n", line);
                continue;
            }
            email_addresses.push_back(line);
        }
    }

    try {
        curlpp::Cleanup cleaner;

        if (!has_team_id) {
            json result = get_team(token, org_name, team_slug);
            if (!result.contains("id")) {
                fmt::print(std::cerr, "cannot obtain the team_id from {:s}\n", team_slug);
                return -1;
            }
            team_id = result["id"];
            fmt::print(std::cout, "team_id for {:s} is {:d}\n", team_slug, team_id);
        }

        if (!result["team-id-only"].as<bool>()) {
            for (std::string email_address : email_addresses) {
                if (result["dry-run"].as<bool>()) {
                    fmt::print("[dry-run] sending an invitation to '{}'\n", email_address);
                } else {
                    send_invitation(token, org_name, team_id, email_address);
                }
                sleep(WAIT_DURATION);
            }
        }

    } catch (curlpp::LogicError &e) {
        std::cout << e.what() << std::endl;
    } catch (curlpp::RuntimeError &e) {
        std::cout << e.what() << std::endl;
    }
    if (email_file.is_open()) {
        email_file.close();
    }

    return 0;
}
