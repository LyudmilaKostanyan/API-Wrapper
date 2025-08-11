# API-Wrapper

## Overview

A tiny, modern C++ wrapper around the C library **libcurl** that provides a safe, convenient HTTP client with **RAII** resource management and a clean API:

* `HttpClient::get(url, headers)`
* `HttpClient::post(url, data, headers)`
* Strong defaults (timeouts, TLS verification, redirects)
* Exceptions for error handling (`HttpError`)
* Works on Linux, macOS, and Windows

## Problem Description

Using raw libcurl in C requires manually creating and cleaning resources (`CURL*`, `curl_slist*`), wiring callbacks, and remembering dozens of `CURLOPT_*` flags for each request. It’s easy to leak resources or forget an option. This project wraps those details in a small C++ class that:

* Initializes/cleans up libcurl automatically (RAII)
* Centralizes options in one place
* Exposes a minimal, readable interface for everyday HTTP

## explanation of some topics in this project (optional)

* **RAII (Resource Acquisition Is Initialization):**
  `HttpClient` owns a `CURL*` and frees it in the destructor; `Slist` owns a `curl_slist*` and frees it automatically. No manual `free_all` calls are needed even if exceptions occur.

* **Thread-safe global init:**
  `curl_global_init` is called once per process using `std::once_flag` to avoid data races.

* **Request lifecycle:**
  Each request resets the handle (`curl_easy_reset`), reapplies current options (timeout, redirects, TLS checks, user-agent), sets method-specific flags, attaches headers, performs the transfer, and returns a `Response {status, body, headers}`.

* **Header & body handling:**
  Write callbacks accumulate response body and parse headers line-by-line into `Response::headers`.

* **Portability notes:**

  * Linux needs `libcurl4-openssl-dev` (or `libcurl4-gnutls-dev`).
  * macOS via Homebrew: `brew install curl` (it’s keg-only; pass `-DCMAKE_PREFIX_PATH="$(brew --prefix curl)"`).
  * Windows: recommended via **vcpkg** (`vcpkg install curl`) and pass `-DCMAKE_TOOLCHAIN_FILE=...`.

## Example Output

```
[GET] Status: 200
[GET] Body length: 267 bytes
[GET] Body preview: {
  "args": {}, 
  "headers": {
    "Accept": "application/json", 
    "Host": "httpbin.org", 
    "User-Agent": "MyApp/1.0", 
    "X-Amzn-Trace-Id": "Root=1-...
  }, 

[POST] Status: 200
[POST] Body length: 484 bytes
[POST] Body preview: {
  "args": {}, 
  "data": "{\"name\":\"Lyudmila\",\"role\":\"Developer\"}", 
  "files": {}, 
  "form": {}, 
  "headers": {
    "Accept": "*/*", 
    "Content-Length": "38", 
    "Content-Type": "appl
```

## Explanation of output

* `Status: 200` — HTTP success.
* `Body length: ...` — size of the **response body** returned by the server.
* `Body preview: ...` — first 200 characters of JSON from `httpbin.org`.

  * For `GET`, httpbin echoes request headers like `Accept` and `User-Agent`.
  * For `POST`, it echoes your JSON payload under `"data"` and your headers (e.g., `Content-Type: application/json`).
  * `Content-Length: 38` is the size of your **request body** sent to the server.

---

## How to Compile and Run

### Clone the Repository

```bash
git clone https://github.com/LyudmilaKostanyan/API-Wrapper.git
cd API-Wrapper
```

### Build

This project uses CMake and links against libcurl. The recommended CMakeLists uses a small library target for the wrapper and an executable for the demo.

**Install dependencies first:**

* **Ubuntu/Debian**

  ```bash
  sudo apt-get update
  sudo apt-get install -y libcurl4-openssl-dev cmake g++
  ```

  (or `libcurl4-gnutls-dev` if you prefer GnuTLS)

* **Fedora**

  ```bash
  sudo dnf install libcurl-devel cmake gcc-c++
  ```

* **macOS (Homebrew)**

  ```bash
  brew update
  brew install curl cmake
  ```

  Homebrew’s curl is keg-only. When configuring CMake, pass:

  ```bash
  cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix curl)"
  ```

* **Windows (MSVC + vcpkg)**

  ```powershell
  git clone https://github.com/microsoft/vcpkg.git
  .\vcpkg\bootstrap-vcpkg.bat
  .\vcpkg\vcpkg.exe install curl
  ```

  Then add `-DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake` to your CMake configure.

**Configure and build:**

* **Linux/macOS (GCC/Clang)**

  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release
  ```

* **macOS (Homebrew curl path)**

  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix curl)"
  cmake --build build --config Release
  ```

* **Windows (MSVC + vcpkg)**

  ```powershell
  cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake"
  cmake --build build --config Release
  ```

### Run

* **Linux/macOS**

  ```bash
  ./build/main
  ```

* **Windows**

  ```powershell
  .\build\Release\main.exe
  ```
