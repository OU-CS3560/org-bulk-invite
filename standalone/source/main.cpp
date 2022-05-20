#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

#include <cxxopts.hpp>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <dotenv.h>

using json = nlohmann::json;

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
    // TODO(kchusap): sanitize the email addresses. skip the email with invalid domain.
    // TODO(kchusap): implement back-off so the program can recover from transient-fault from the network.
    // TODO(kchusap): give enough time between request, so that we are not rate limitted.

    // Construct the payload.
    ghapi::payload p {"bobcat@ohio.edu", "direct_member", std::vector<int>{team_id} };
    json payload_j = p;
    std::string body = payload_j.dump();
    std::cout << payload_j.dump();

    try {
        curlpp::Cleanup cleaner;
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

        request.perform();

        std::cout << response.str() << std::endl;

    } catch (curlpp::LogicError &e) {
        std::cout << e.what() << std::endl;
    } catch (curlpp::RuntimeError &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
