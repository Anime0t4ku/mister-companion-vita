#include "wallpaper_db.hpp"

#include <zlib.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

static std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static bool endsWithLower(const std::string& value, const std::string& suffix) {
    std::string a = lower(value);
    std::string b = lower(suffix);
    return a.size() >= b.size() && a.compare(a.size() - b.size(), b.size(), b) == 0;
}

static std::string basenameOf(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

static bool isImageName(const std::string& name) {
    std::string n = lower(name);
    return endsWithLower(n, ".png") || endsWithLower(n, ".jpg") || endsWithLower(n, ".jpeg");
}

static uint16_t le16(const std::vector<unsigned char>& data, size_t pos) {
    if (pos + 1 >= data.size()) return 0;
    return static_cast<uint16_t>(data[pos]) | (static_cast<uint16_t>(data[pos + 1]) << 8);
}

static uint32_t le32(const std::vector<unsigned char>& data, size_t pos) {
    if (pos + 3 >= data.size()) return 0;
    return static_cast<uint32_t>(data[pos]) |
           (static_cast<uint32_t>(data[pos + 1]) << 8) |
           (static_cast<uint32_t>(data[pos + 2]) << 16) |
           (static_cast<uint32_t>(data[pos + 3]) << 24);
}

static bool parseUrl(const std::string& url, std::string& host, std::string& path, std::string& error) {
    const std::string prefix = "https://";
    if (url.rfind(prefix, 0) != 0) {
        error = "Only HTTPS wallpaper DB URLs are supported.";
        return false;
    }
    size_t start = prefix.size();
    size_t slash = url.find('/', start);
    if (slash == std::string::npos) {
        host = url.substr(start);
        path = "/";
    } else {
        host = url.substr(start, slash - start);
        path = url.substr(slash);
    }
    if (host.empty()) {
        error = "Invalid wallpaper DB URL.";
        return false;
    }
    return true;
}

static std::string mbedtlsError(int rc) {
    char buffer[128] = {0};
    mbedtls_strerror(rc, buffer, sizeof(buffer));
    return std::string(buffer);
}

static bool decodeChunked(const std::string& input, std::string& output) {
    size_t pos = 0;
    output.clear();
    while (pos < input.size()) {
        size_t lineEnd = input.find("\r\n", pos);
        if (lineEnd == std::string::npos) return false;
        std::string hex = input.substr(pos, lineEnd - pos);
        size_t semi = hex.find(';');
        if (semi != std::string::npos) hex = hex.substr(0, semi);
        unsigned long size = 0;
        std::stringstream ss;
        ss << std::hex << hex;
        ss >> size;
        pos = lineEnd + 2;
        if (size == 0) return true;
        if (pos + size > input.size()) return false;
        output.append(input.data() + pos, size);
        pos += size;
        if (pos + 2 > input.size()) return false;
        if (input[pos] == '\r' && input[pos + 1] == '\n') pos += 2;
    }
    return false;
}

static bool httpsGet(const std::string& url, std::vector<unsigned char>& body, std::string& error, int redirects = 0) {
    if (redirects > 3) {
        error = "Too many redirects.";
        return false;
    }

    std::string host;
    std::string path;
    if (!parseUrl(url, host, path, error)) return false;

    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbg;

    mbedtls_net_init(&net);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbg);

    const char* personal = "mc-nx-wallpaper";
    int rc = mbedtls_ctr_drbg_seed(&ctrDrbg, mbedtls_entropy_func, &entropy,
                                   reinterpret_cast<const unsigned char*>(personal), std::strlen(personal));
    if (rc != 0) {
        error = "TLS seed failed: " + mbedtlsError(rc);
        goto fail;
    }

    rc = mbedtls_net_connect(&net, host.c_str(), "443", MBEDTLS_NET_PROTO_TCP);
    if (rc != 0) {
        error = "Connection failed: " + mbedtlsError(rc);
        goto fail;
    }

    rc = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc != 0) {
        error = "TLS config failed: " + mbedtlsError(rc);
        goto fail;
    }

    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctrDrbg);

    rc = mbedtls_ssl_setup(&ssl, &conf);
    if (rc != 0) {
        error = "TLS setup failed: " + mbedtlsError(rc);
        goto fail;
    }

    mbedtls_ssl_set_hostname(&ssl, host.c_str());
    mbedtls_ssl_set_bio(&ssl, &net, mbedtls_net_send, mbedtls_net_recv, nullptr);

    while ((rc = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (rc != MBEDTLS_ERR_SSL_WANT_READ && rc != MBEDTLS_ERR_SSL_WANT_WRITE) {
            error = "TLS handshake failed: " + mbedtlsError(rc);
            goto fail;
        }
    }

    {
        std::string request = "GET " + path + " HTTP/1.1\r\n"
                              "Host: " + host + "\r\n"
                              "User-Agent: MiSTer-Companion-Vita\r\n"
                              "Accept: */*\r\n"
                              "Connection: close\r\n\r\n";
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(request.data());
        size_t remaining = request.size();
        while (remaining > 0) {
            rc = mbedtls_ssl_write(&ssl, ptr, remaining);
            if (rc > 0) {
                ptr += rc;
                remaining -= static_cast<size_t>(rc);
            } else if (rc != MBEDTLS_ERR_SSL_WANT_READ && rc != MBEDTLS_ERR_SSL_WANT_WRITE) {
                error = "HTTP write failed: " + mbedtlsError(rc);
                goto fail;
            }
        }
    }

    {
        std::string response;
        unsigned char buffer[4096];
        while (true) {
            rc = mbedtls_ssl_read(&ssl, buffer, sizeof(buffer));
            if (rc > 0) {
                response.append(reinterpret_cast<const char*>(buffer), static_cast<size_t>(rc));
                continue;
            }
            if (rc == 0 || rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) break;
            if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
            error = "HTTP read failed: " + mbedtlsError(rc);
            goto fail;
        }

        size_t headerEnd = response.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            error = "Invalid HTTP response.";
            goto fail;
        }
        std::string header = response.substr(0, headerEnd);
        std::string rawBody = response.substr(headerEnd + 4);

        int status = 0;
        if (header.rfind("HTTP/", 0) == 0) {
            size_t firstSpace = header.find(' ');
            if (firstSpace != std::string::npos) status = std::atoi(header.c_str() + firstSpace + 1);
        }

        if (status >= 300 && status < 400) {
            std::string location;
            std::stringstream hs(header);
            std::string line;
            while (std::getline(hs, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                std::string l = lower(line);
                if (l.rfind("location:", 0) == 0) {
                    location = line.substr(9);
                    while (!location.empty() && std::isspace(static_cast<unsigned char>(location.front()))) location.erase(location.begin());
                    break;
                }
            }
            if (!location.empty()) {
                mbedtls_ssl_close_notify(&ssl);
                mbedtls_net_free(&net);
                mbedtls_ssl_free(&ssl);
                mbedtls_ssl_config_free(&conf);
                mbedtls_ctr_drbg_free(&ctrDrbg);
                mbedtls_entropy_free(&entropy);
                return httpsGet(location, body, error, redirects + 1);
            }
        }

        if (status < 200 || status >= 300) {
            error = "HTTP error " + std::to_string(status) + ".";
            goto fail;
        }

        std::string decoded;
        if (lower(header).find("transfer-encoding: chunked") != std::string::npos) {
            if (!decodeChunked(rawBody, decoded)) {
                error = "Unable to decode chunked response.";
                goto fail;
            }
        } else {
            decoded = rawBody;
        }

        body.assign(decoded.begin(), decoded.end());
    }

    mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&net);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_entropy_free(&entropy);
    return true;

fail:
    mbedtls_ssl_close_notify(&ssl);
    mbedtls_net_free(&net);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_entropy_free(&entropy);
    return false;
}

static bool inflateRaw(const unsigned char* input, size_t inputSize, size_t outputSize, std::string& output) {
    output.assign(outputSize, '\0');
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input));
    stream.avail_in = static_cast<uInt>(inputSize);
    stream.next_out = reinterpret_cast<Bytef*>(&output[0]);
    stream.avail_out = static_cast<uInt>(output.size());
    int rc = inflateInit2(&stream, -MAX_WBITS);
    if (rc != Z_OK) return false;
    rc = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    if (rc != Z_STREAM_END) return false;
    output.resize(stream.total_out);
    return true;
}

static bool extractJsonFromZip(const std::vector<unsigned char>& zip, std::string& json, std::string& error) {
    if (zip.empty()) {
        error = "Empty wallpaper database.";
        return false;
    }

    if (zip.size() >= 1 && (zip[0] == '{' || zip[0] == '[')) {
        json.assign(zip.begin(), zip.end());
        return true;
    }

    size_t eocd = std::string::npos;
    const size_t minSearch = zip.size() > 66000 ? zip.size() - 66000 : 0;
    for (size_t pos = zip.size() >= 22 ? zip.size() - 22 : 0; pos >= minSearch && pos < zip.size(); --pos) {
        if (le32(zip, pos) == 0x06054b50) {
            eocd = pos;
            break;
        }
        if (pos == 0) break;
    }
    if (eocd == std::string::npos) {
        error = "Invalid ZIP database.";
        return false;
    }

    uint16_t entries = le16(zip, eocd + 10);
    uint32_t cdOffset = le32(zip, eocd + 16);
    size_t pos = cdOffset;

    for (uint16_t i = 0; i < entries && pos + 46 <= zip.size(); ++i) {
        if (le32(zip, pos) != 0x02014b50) break;
        uint16_t method = le16(zip, pos + 10);
        uint32_t compSize = le32(zip, pos + 20);
        uint32_t uncompSize = le32(zip, pos + 24);
        uint16_t nameLen = le16(zip, pos + 28);
        uint16_t extraLen = le16(zip, pos + 30);
        uint16_t commentLen = le16(zip, pos + 32);
        uint32_t localOffset = le32(zip, pos + 42);
        if (pos + 46 + nameLen > zip.size()) break;
        std::string name(reinterpret_cast<const char*>(&zip[pos + 46]), nameLen);
        pos += 46 + nameLen + extraLen + commentLen;

        if (!endsWithLower(name, ".json")) continue;
        if (localOffset + 30 > zip.size() || le32(zip, localOffset) != 0x04034b50) continue;
        uint16_t localNameLen = le16(zip, localOffset + 26);
        uint16_t localExtraLen = le16(zip, localOffset + 28);
        size_t dataOffset = localOffset + 30 + localNameLen + localExtraLen;
        if (dataOffset + compSize > zip.size()) continue;

        if (method == 0) {
            json.assign(reinterpret_cast<const char*>(&zip[dataOffset]), compSize);
            return true;
        }
        if (method == 8) {
            if (inflateRaw(&zip[dataOffset], compSize, uncompSize, json)) return true;
        }
    }

    error = "No readable JSON file in wallpaper DB.";
    return false;
}

static std::string jsonUnescape(const std::string& value) {
    std::string out;
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '\\' && i + 1 < value.size()) {
            char n = value[++i];
            switch (n) {
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                default: out.push_back(n); break;
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

static bool isUrlSafe(char c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return true;
    switch (c) {
        case '/': case '-': case '_': case '.': case '~': case '(': case ')': case '[': case ']': case '!': return true;
        default: return false;
    }
}

static std::string urlEncodePath(const std::string& path) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : path) {
        if (isUrlSafe(static_cast<char>(c))) out.push_back(static_cast<char>(c));
        else {
            out.push_back('%');
            out.push_back(hex[(c >> 4) & 0xF]);
            out.push_back(hex[c & 0xF]);
        }
    }
    return out;
}

static std::string makeUrl(const std::string& rawBase, const std::string& repoPath) {
    std::string base = rawBase;
    while (!base.empty() && base.back() == '/') base.pop_back();
    std::string path = repoPath;
    while (!path.empty() && path.front() == '/') path.erase(path.begin());
    return base + "/" + urlEncodePath(path);
}

static bool matchesFilter(const std::string& name, const std::string& filterMode) {
    std::string n = lower(name);
    bool is43 = n.find("4x3") != std::string::npos || n.find("4:3") != std::string::npos || n.find("4-3") != std::string::npos;
    if (filterMode == "169") return !is43;
    if (filterMode == "43") return is43;
    return true;
}

static std::string valueNear(const std::string& json, size_t start, const std::string& key) {
    size_t limit = std::min(json.size(), start + static_cast<size_t>(600));
    std::string needle = "\"" + key + "\"";
    size_t k = json.find(needle, start);
    if (k == std::string::npos || k > limit) return "";
    size_t colon = json.find(':', k + needle.size());
    if (colon == std::string::npos || colon > limit) return "";
    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos || q1 > limit) return "";
    size_t q2 = q1 + 1;
    bool escaped = false;
    while (q2 < json.size()) {
        char c = json[q2];
        if (escaped) escaped = false;
        else if (c == '\\') escaped = true;
        else if (c == '"') break;
        ++q2;
    }
    if (q2 >= json.size()) return "";
    return jsonUnescape(json.substr(q1 + 1, q2 - q1 - 1));
}

static void addEntry(std::vector<WallpaperDb::Entry>& entries, std::set<std::string>& seen,
                     const std::string& repoPath, const std::string& rawBase, const std::string& urlCandidate,
                     const std::string& filterMode) {
    if (repoPath.empty()) return;
    std::string path = repoPath;
    while (!path.empty() && std::isspace(static_cast<unsigned char>(path.front()))) path.erase(path.begin());
    while (!path.empty() && std::isspace(static_cast<unsigned char>(path.back()))) path.pop_back();
    std::string name = basenameOf(path);
    if (!isImageName(name)) return;
    if (!matchesFilter(name, filterMode)) return;
    std::string key = lower(name);
    if (seen.count(key)) return;
    std::string url = urlCandidate;
    if (url.empty()) url = makeUrl(rawBase, path);
    entries.push_back({name, url});
    seen.insert(key);
}

static std::vector<WallpaperDb::Entry> parseWallpaperJson(const std::string& json, const std::string& rawBase, const std::string& filterMode) {
    std::vector<WallpaperDb::Entry> entries;
    std::set<std::string> seen;
    bool escaped = false;
    for (size_t i = 0; i < json.size(); ++i) {
        if (json[i] != '"') continue;
        size_t start = i + 1;
        size_t end = start;
        escaped = false;
        while (end < json.size()) {
            char c = json[end];
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') break;
            ++end;
        }
        if (end >= json.size()) break;
        std::string token = jsonUnescape(json.substr(start, end - start));
        i = end;
        if (!isImageName(basenameOf(token))) continue;

        size_t after = end + 1;
        while (after < json.size() && std::isspace(static_cast<unsigned char>(json[after]))) ++after;
        std::string url;
        if (after < json.size() && json[after] == ':') {
            url = valueNear(json, after, "url");
            if (url.empty()) url = valueNear(json, after, "raw_url");
            if (url.empty()) url = valueNear(json, after, "download_url");
        }
        addEntry(entries, seen, token, rawBase, url, filterMode);
    }
    return entries;
}

} // namespace

namespace WallpaperDb {

bool parseEntriesFromData(const std::vector<unsigned char>& data,
                          const std::string& rawBase,
                          const std::string& filterMode,
                          std::vector<Entry>& entries,
                          std::string& error) {
    entries.clear();

    std::string json;
    if (!extractJsonFromZip(data, json, error)) return false;

    entries = parseWallpaperJson(json, rawBase, filterMode);
    if (entries.empty()) {
        error = "No wallpapers found in database.";
        return false;
    }
    return true;
}

bool fetchEntries(const std::string& dbUrl,
                  const std::string& rawBase,
                  const std::string& filterMode,
                  std::vector<Entry>& entries,
                  std::string& error) {
    entries.clear();
    std::vector<unsigned char> body;
    if (!httpsGet(dbUrl, body, error)) return false;

    return parseEntriesFromData(body, rawBase, filterMode, entries, error);
}

} // namespace WallpaperDb
