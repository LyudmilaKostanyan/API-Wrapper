#include "http_client.hpp"
#include <sstream>
#include <cstring>

namespace net {

static std::once_flag g_curl_once;

void HttpClient::global_init_once() {
    std::call_once(g_curl_once, []{
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
            throw HttpError("curl_global_init failed");
    });
}

HttpClient::HttpClient(Options opt) : opt_(std::move(opt)) {
    global_init_once();
    h_ = curl_easy_init();
    if (!h_) throw HttpError("curl_easy_init failed");
    apply_common_options();
}

HttpClient::HttpClient() : HttpClient(Options{}) {}

HttpClient::~HttpClient() {
    if (h_) curl_easy_cleanup(h_);
}

HttpClient::HttpClient(HttpClient&& other) noexcept : h_(other.h_), opt_(other.opt_) {
    other.h_ = nullptr;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        if (h_) curl_easy_cleanup(h_);
        h_ = other.h_;
        opt_ = other.opt_;
        other.h_ = nullptr;
    }
    return *this;
}

void HttpClient::set_options(const Options& opt) {
    opt_ = opt;
    apply_common_options();
}

void HttpClient::apply_common_options() {
    curl_easy_reset(h_);
    // Basic callbacks
    curl_easy_setopt(h_, CURLOPT_WRITEFUNCTION, &HttpClient::write_body_cb);
    curl_easy_setopt(h_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(h_, CURLOPT_HEADERFUNCTION, &HttpClient::write_header_cb);
    curl_easy_setopt(h_, CURLOPT_HEADERDATA, this);

    // Timeout / redirects / user-agent
    curl_easy_setopt(h_, CURLOPT_TIMEOUT_MS, opt_.timeout_ms);
    curl_easy_setopt(h_, CURLOPT_FOLLOWLOCATION, opt_.follow_redirects ? 1L : 0L);
    if (opt_.user_agent && !opt_.user_agent->empty())
        curl_easy_setopt(h_, CURLOPT_USERAGENT, opt_.user_agent->c_str());

    // TLS verification
    curl_easy_setopt(h_, CURLOPT_SSL_VERIFYPEER, opt_.verify_peer ? 1L : 0L);
    curl_easy_setopt(h_, CURLOPT_SSL_VERIFYHOST, opt_.verify_host ? 2L : 0L);
}

size_t HttpClient::write_body_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto self = static_cast<HttpClient*>(userdata);
    self->body_acc_.append(ptr, size * nmemb);
    return size * nmemb;
}

static inline std::pair<std::string,std::string> split_header_line(const std::string& line) {
    auto pos = line.find(':');
    if (pos == std::string::npos) return {line, ""};
    // Trim spaces after ':'
    size_t start = pos + 1;
    while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) ++start;
    return { line.substr(0, pos), line.substr(start) };
}

size_t HttpClient::write_header_cb(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto self = static_cast<HttpClient*>(userdata);
    const size_t total = size * nitems;
    // Each header line comes with \r\n
    std::string line(buffer, total);
    if (line.rfind("HTTP/", 0) == 0) {
        // New status section — clear accumulated headers (redirects / multi-stage responses)
        self->hdr_acc_.clear();
    } else if (!line.empty() && line != "\r\n") {
        auto kv = split_header_line(line.substr(0, line.find("\r\n")));
        self->hdr_acc_.push_back(std::move(kv));
    }
    return total;
}

Response HttpClient::perform_with_headers_and_body() {
    body_acc_.clear();
    hdr_acc_.clear();

    const auto res = curl_easy_perform(h_);
    if (res != CURLE_OK) {
        std::ostringstream oss;
        oss << "curl_easy_perform failed: " << curl_easy_strerror(res);
        throw HttpError(oss.str());
    }
    long code = 0;
    curl_easy_getinfo(h_, CURLINFO_RESPONSE_CODE, &code);

    Response r;
    r.status = code;
    r.body = std::move(body_acc_);
    r.headers = std::move(hdr_acc_);
    return r;
}

Response HttpClient::get(const std::string& url,
                         const std::vector<std::pair<std::string,std::string>>& headers) {
    apply_common_options();
    curl_easy_setopt(h_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(h_, CURLOPT_URL, url.c_str());

    Slist sl;
    for (auto& [k,v] : headers) {
        sl.add(k + ": " + v);
    }
    if (sl.ptr) curl_easy_setopt(h_, CURLOPT_HTTPHEADER, sl.ptr);

    return perform_with_headers_and_body();
}

Response HttpClient::post(const std::string& url,
                          std::string_view data,
                          const std::vector<std::pair<std::string,std::string>>& headers) {
    apply_common_options();
    curl_easy_setopt(h_, CURLOPT_POST, 1L);
    curl_easy_setopt(h_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h_, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(h_, CURLOPT_POSTFIELDSIZE, static_cast<long>(data.size()));

    Slist sl;
    for (auto& [k,v] : headers) {
        sl.add(k + ": " + v);
    }
    if (sl.ptr) curl_easy_setopt(h_, CURLOPT_HTTPHEADER, sl.ptr);

    return perform_with_headers_and_body();
}

} // namespace net
