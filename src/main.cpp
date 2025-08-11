#include "http_client.hpp"
#include <iostream>

int main() {
    try {
        net::HttpClient::Options opt;
        opt.timeout_ms = 10000;
        opt.follow_redirects = true;
        opt.user_agent = std::string("MyApp/1.0");

        net::HttpClient client(opt);

        auto getResp = client.get("https://httpbin.org/get",
                                  {{"Accept", "application/json"}});
        std::cout << "[GET] Status: " << getResp.status << "\n";
        std::cout << "[GET] Body length: " << getResp.body.size() << " bytes\n";
        std::cout << "[GET] Body preview: " << getResp.body.substr(0, 200) << "\n\n";

        std::string postData = R"({"name":"Lyudmila","role":"Developer"})";
        auto postResp = client.post("https://httpbin.org/post",
                                    postData,
                                    {{"Content-Type", "application/json"}});
        std::cout << "[POST] Status: " << postResp.status << "\n";
        std::cout << "[POST] Body length: " << postResp.body.size() << " bytes\n";
        std::cout << "[POST] Body preview: " << postResp.body.substr(0, 200) << "\n";
    } catch (const net::HttpError& e) {
        std::cerr << "HTTP Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected Error: " << e.what() << "\n";
        return 1;
    }
}
