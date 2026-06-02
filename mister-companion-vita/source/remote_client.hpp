#pragma once

#include <string>

class RemoteClient {
public:
    RemoteClient();
    ~RemoteClient();

    bool connect(const std::string& host, std::string& message);
    void disconnect();
    bool isConnected() const;

    bool sendController(const std::string& control, const std::string& name, const std::string& action, std::string& message);
    bool releaseAll(std::string& message);
    bool ping(std::string& message);

private:
    int socketFd = -1;

    bool sendText(const std::string& payload, std::string& message);
    std::string makeWebSocketKey() const;
};
