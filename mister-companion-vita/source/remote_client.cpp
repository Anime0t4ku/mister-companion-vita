#include "remote_client.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <vector>

static std::string socketErrorText() {
    return std::string(strerror(errno));
}

static std::string jsonMessage(const std::string& type, const std::string& a, const std::string& b, const std::string& c) {
    if (type == "controller") {
        return "{\"type\":\"controller\",\"control\":\"" + a + "\",\"name\":\"" + b + "\",\"action\":\"" + c + "\"}";
    }
    if (type == "system") {
        return "{\"type\":\"system\",\"command\":\"" + a + "\"}";
    }
    return "{\"type\":\"ping\"}";
}

RemoteClient::RemoteClient() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

RemoteClient::~RemoteClient() {
    disconnect();
}

bool RemoteClient::isConnected() const {
    return socketFd >= 0;
}

void RemoteClient::disconnect() {
    if (socketFd >= 0) {
        close(socketFd);
        socketFd = -1;
    }
}

std::string RemoteClient::makeWebSocketKey() const {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char bytes[16];
    for (unsigned char& b : bytes) b = static_cast<unsigned char>(std::rand() & 0xff);

    std::string out;
    int value = 0;
    int bits = -6;
    for (unsigned char c : bytes) {
        value = (value << 8) + c;
        bits += 8;
        while (bits >= 0) {
            out.push_back(table[(value >> bits) & 0x3f]);
            bits -= 6;
        }
    }
    if (bits > -6) out.push_back(table[((value << 8) >> (bits + 8)) & 0x3f]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

bool RemoteClient::connect(const std::string& host, std::string& message) {
    disconnect();

    if (host.empty()) {
        message = "MiSTer IP is empty.";
        return false;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int gai = getaddrinfo(host.c_str(), "9191", &hints, &result);
    if (gai != 0 || !result) {
        message = "Unable to resolve MiSTer host.";
        return false;
    }

    socketFd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socketFd < 0) {
        freeaddrinfo(result);
        message = "Unable to create remote socket: " + socketErrorText();
        return false;
    }

    if (::connect(socketFd, result->ai_addr, result->ai_addrlen) != 0) {
        freeaddrinfo(result);
        message = "Remote daemon is not reachable on port 9191.";
        disconnect();
        return false;
    }

    freeaddrinfo(result);

    const std::string key = makeWebSocketKey();
    const std::string request =
        "GET /remote/v1 HTTP/1.1\r\n"
        "Host: " + host + ":9191\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";

    ssize_t sent = ::send(socketFd, request.c_str(), request.size(), 0);
    if (sent < 0 || static_cast<size_t>(sent) != request.size()) {
        message = "Unable to send WebSocket handshake.";
        disconnect();
        return false;
    }

    char response[1024] = {0};
    ssize_t bytes = recv(socketFd, response, sizeof(response) - 1, 0);
    if (bytes <= 0) {
        message = "No WebSocket handshake response.";
        disconnect();
        return false;
    }

    std::string text(response, response + bytes);
    if (text.find("101") == std::string::npos) {
        message = "Remote daemon rejected WebSocket handshake.";
        disconnect();
        return false;
    }

    message = "Remote daemon connected.";
    return true;
}

bool RemoteClient::sendText(const std::string& payload, std::string& message) {
    if (!isConnected()) {
        message = "Remote daemon is not connected.";
        return false;
    }

    std::vector<unsigned char> frame;
    frame.push_back(0x81);

    const size_t length = payload.size();
    if (length < 126) {
        frame.push_back(static_cast<unsigned char>(0x80 | length));
    } else if (length <= 65535) {
        frame.push_back(0x80 | 126);
        frame.push_back(static_cast<unsigned char>((length >> 8) & 0xff));
        frame.push_back(static_cast<unsigned char>(length & 0xff));
    } else {
        message = "Remote message is too large.";
        return false;
    }

    unsigned char mask[4];
    for (unsigned char& b : mask) b = static_cast<unsigned char>(std::rand() & 0xff);
    frame.insert(frame.end(), mask, mask + 4);

    for (size_t i = 0; i < payload.size(); i++) {
        frame.push_back(static_cast<unsigned char>(payload[i]) ^ mask[i % 4]);
    }

    ssize_t sent = ::send(socketFd, frame.data(), frame.size(), 0);
    if (sent < 0 || static_cast<size_t>(sent) != frame.size()) {
        message = "Unable to send remote command.";
        disconnect();
        return false;
    }

    message = "Remote command sent.";
    return true;
}

bool RemoteClient::sendController(const std::string& control, const std::string& name, const std::string& action, std::string& message) {
    return sendText(jsonMessage("controller", control, name, action), message);
}

bool RemoteClient::releaseAll(std::string& message) {
    return sendText(jsonMessage("system", "release_all", "", ""), message);
}

bool RemoteClient::ping(std::string& message) {
    return sendText(jsonMessage("ping", "", "", ""), message);
}
