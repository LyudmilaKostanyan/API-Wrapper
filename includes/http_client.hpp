#pragma once
#include <curl/curl.h>
#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <stdexcept>
#include <mutex>

namespace net {

struct Response {
    long status = 0;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
};

class HttpError : public std::runtime_error {
public:
    explicit HttpError(const std::string& what) : std::runtime_error(what) {}
};

class HttpClient {
public:
    struct Options {
        long timeout_ms;
        bool follow_redirects;
        std::optional<std::string> user_agent;
        bool verify_peer;   // TLS peer verification
        bool verify_host;   // TLS host verification
        Options()
            : timeout_ms(15000),
              follow_redirects(true),
              user_agent(std::string("HttpClient/1.0")),
              verify_peer(true),
              verify_host(true) {}
    };

    HttpClient();
    explicit HttpClient(Options opt);
    ~HttpClient();

    // Non-copyable, moveable (resource-owning class)
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;

    // High-level methods
    Response get(const std::string& url,
                 const std::vector<std::pair<std::string,std::string>>& headers = {});

    Response post(const std::string& url,
                  std::string_view data,
                  const std::vector<std::pair<std::string,std::string>>& headers = {});

    // Allows changing options at runtime
    void set_options(const Options& opt);

private:
    // RAII wrapper for curl_slist*
    struct Slist {
        curl_slist* ptr = nullptr;
        ~Slist() { if (ptr) curl_slist_free_all(ptr); }
        Slist(const Slist&) = delete;
        Slist& operator=(const Slist&) = delete;
        Slist() = default;
        Slist(Slist&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }
        Slist& operator=(Slist&& other) noexcept { if (this != &other) { if (ptr) curl_slist_free_all(ptr); ptr = other.ptr; other.ptr = nullptr; } return *this; }
        void add(const std::string& h) { ptr = curl_slist_append(ptr, h.c_str()); }
    };

    // Thread-safe global initialization of libcurl
    static void global_init_once();

    static size_t write_body_cb(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t write_header_cb(char* buffer, size_t size, size_t nitems, void* userdata);

    void apply_common_options();
    Response perform_with_headers_and_body();

private:
    CURL* h_ = nullptr;
    Options opt_;
    std::string body_acc_;
    std::vector<std::pair<std::string, std::string>> hdr_acc_;
};

} // namespace net
