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

#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <nlohmann/json.hpp>
#include <dotenv.h>
#include <rapidcsv.h>

using json = nlohmann::json;

const int WAIT_DURATION = 5; // [s]

namespace ghapi {
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

void send_invitation(const std::string& token, const std::string &org_name, int team_id, const std::string& email_address) {
    // Construct the payload.
    ghapi::payload p {email_address, "direct_member", std::vector<int>{team_id} };
    json payload_j = p;
    std::string body = payload_j.dump();
    std::cout << payload_j.dump();

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
            request.perform();
            is_sent = true;
            std::cout << response.str() << std::endl;

            // TODO: Propoerly handle `Retry-After` if one is received.
            // TODO: Check for 4xx or 5xx and handle the errors properly.
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
    try {
        team_id = std::stoi(dotenv::env["TEAM_ID"], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        std::cerr << "invalid text for a team_id\n";
        return -1;
    } catch (std::out_of_range const& ex) {
        std::cerr << "value of team_id is out of range\n";
        return -1;
    }

    std::string input_filename;
    cxxopts::Options options("BulkInvite", "Send multiple invites to GitHub organization");
    // clang-format off
    options.add_options()
      ("h,help", "Show help")
      ("f,file", "Input file name", cxxopts::value(input_filename)->default_value("email_addresses.txt"));
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
        for (std::string email_address : email_addresses) {
            fmt::print("sending an invitation to '{}'\n", email_address);
            // send_invitation(token, org_name, team_id, line);

            sleep(WAIT_DURATION);
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
