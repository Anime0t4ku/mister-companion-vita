#include "ssh_client.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <libssh2.h>
#include <libssh2_sftp.h>

static std::string lastSocketError() {
    return std::string(strerror(errno));
}

SshClient::SshClient() = default;

SshClient::~SshClient() {
    disconnect();
}

bool SshClient::isConnected() const {
    return session != nullptr && socketFd >= 0;
}

void SshClient::disconnect() {
    if (session) {
        libssh2_session_disconnect(static_cast<LIBSSH2_SESSION*>(session), "MiSTer Companion Vita disconnect");
        libssh2_session_free(static_cast<LIBSSH2_SESSION*>(session));
        session = nullptr;
    }

    if (socketFd >= 0) {
        close(socketFd);
        socketFd = -1;
    }
}

bool SshClient::connect(const AppConfig& config, std::string& message) {
    disconnect();

    if (config.host.empty()) {
        message = "MiSTer IP is empty.";
        return false;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int gai = getaddrinfo(config.host.c_str(), "22", &hints, &result);
    if (gai != 0 || !result) {
        message = "Unable to resolve host.";
        return false;
    }

    socketFd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socketFd < 0) {
        freeaddrinfo(result);
        message = "Unable to create socket: " + lastSocketError();
        return false;
    }

    if (::connect(socketFd, result->ai_addr, result->ai_addrlen) != 0) {
        freeaddrinfo(result);
        message = "Unable to connect to SSH port 22: " + lastSocketError();
        disconnect();
        return false;
    }

    freeaddrinfo(result);

    LIBSSH2_SESSION* sshSession = libssh2_session_init();
    if (!sshSession) {
        message = "Unable to create SSH session.";
        disconnect();
        return false;
    }

    libssh2_session_set_blocking(sshSession, 1);

    if (libssh2_session_handshake(sshSession, socketFd) != 0) {
        message = "SSH handshake failed.";
        libssh2_session_free(sshSession);
        session = nullptr;
        disconnect();
        return false;
    }

    const std::string username = config.username.empty() ? "root" : config.username;
    const std::string password = config.password.empty() ? "1" : config.password;

    if (libssh2_userauth_password(sshSession, username.c_str(), password.c_str()) != 0) {
        message = "SSH login failed.";
        libssh2_session_disconnect(sshSession, "login failed");
        libssh2_session_free(sshSession);
        session = nullptr;
        disconnect();
        return false;
    }

    session = sshSession;
    activeConfig = config;

    SshResult test = runCommand("echo connected");
    if (!test.success) {
        message = test.error.empty() ? "SSH command test failed." : test.error;
        disconnect();
        return false;
    }

    message = test.output.empty() ? "Connected successfully." : test.output;
    return true;
}


bool SshClient::readRemoteFile(const std::string& path, std::vector<unsigned char>& data, std::string& error, size_t maxBytes) {
    data.clear();
    error.clear();

    if (!isConnected()) {
        error = "No active SSH connection.";
        return false;
    }

    LIBSSH2_SESSION* sshSession = static_cast<LIBSSH2_SESSION*>(session);
    LIBSSH2_SFTP* sftp = libssh2_sftp_init(sshSession);
    if (!sftp) {
        error = "Unable to start SFTP session.";
        return false;
    }

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(sftp, path.c_str(), LIBSSH2_FXF_READ, 0);
    if (!handle) {
        error = "Unable to open remote file.";
        libssh2_sftp_shutdown(sftp);
        return false;
    }

    char buffer[8192];
    for (;;) {
        ssize_t bytes = libssh2_sftp_read(handle, buffer, sizeof(buffer));
        if (bytes > 0) {
            if (data.size() + static_cast<size_t>(bytes) > maxBytes) {
                error = "Preview image is too large.";
                libssh2_sftp_close(handle);
                libssh2_sftp_shutdown(sftp);
                data.clear();
                return false;
            }
            data.insert(data.end(), buffer, buffer + bytes);
            continue;
        }
        if (bytes == 0) break;
        if (bytes == LIBSSH2_ERROR_EAGAIN) continue;
        error = "Unable to read remote file.";
        libssh2_sftp_close(handle);
        libssh2_sftp_shutdown(sftp);
        data.clear();
        return false;
    }

    libssh2_sftp_close(handle);
    libssh2_sftp_shutdown(sftp);

    if (data.empty()) {
        error = "Remote file is empty.";
        return false;
    }

    return true;
}

SshResult SshClient::runCommand(const std::string& command) {
    SshResult result;

    if (!isConnected()) {
        result.error = "No active SSH connection.";
        return result;
    }

    LIBSSH2_SESSION* sshSession = static_cast<LIBSSH2_SESSION*>(session);
    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(sshSession);
    if (!channel) {
        result.error = "Unable to open SSH channel.";
        disconnect();
        return result;
    }

    int rc = libssh2_channel_exec(channel, command.c_str());
    if (rc != 0) {
        result.error = "Unable to execute SSH command.";
        libssh2_channel_free(channel);
        return result;
    }

    char buffer[1024];
    for (;;) {
        ssize_t bytes = libssh2_channel_read(channel, buffer, sizeof(buffer));
        if (bytes > 0) {
            result.output.append(buffer, buffer + bytes);
            continue;
        }
        if (bytes == LIBSSH2_ERROR_EAGAIN) continue;
        break;
    }

    std::string stderrText;
    for (;;) {
        ssize_t bytes = libssh2_channel_read_stderr(channel, buffer, sizeof(buffer));
        if (bytes > 0) {
            stderrText.append(buffer, buffer + bytes);
            continue;
        }
        if (bytes == LIBSSH2_ERROR_EAGAIN) continue;
        break;
    }

    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);

    int exitCode = libssh2_channel_get_exit_status(channel);
    libssh2_channel_free(channel);

    while (!result.output.empty() && (result.output.back() == '\n' || result.output.back() == '\r')) {
        result.output.pop_back();
    }
    while (!stderrText.empty() && (stderrText.back() == '\n' || stderrText.back() == '\r')) {
        stderrText.pop_back();
    }

    result.success = exitCode == 0;
    result.error = stderrText;
    if (!result.success && result.error.empty()) {
        result.error = "Command exited with code " + std::to_string(exitCode) + ".";
    }

    return result;
}

SshResult SshClient::runCommandStreaming(const std::string& command, const std::function<void(const std::string&)>& onOutput) {
    SshResult result;

    if (!isConnected()) {
        result.error = "No active SSH connection.";
        return result;
    }

    LIBSSH2_SESSION* sshSession = static_cast<LIBSSH2_SESSION*>(session);
    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(sshSession);
    if (!channel) {
        result.error = "Unable to open SSH channel.";
        disconnect();
        return result;
    }

    int rc = libssh2_channel_exec(channel, command.c_str());
    if (rc != 0) {
        result.error = "Unable to execute SSH command.";
        libssh2_channel_free(channel);
        return result;
    }

    char buffer[1024];
    for (;;) {
        ssize_t bytes = libssh2_channel_read(channel, buffer, sizeof(buffer));
        if (bytes > 0) {
            std::string chunk(buffer, buffer + bytes);
            result.output += chunk;
            if (onOutput) onOutput(chunk);
            continue;
        }
        if (bytes == LIBSSH2_ERROR_EAGAIN) continue;
        break;
    }

    std::string stderrText;
    for (;;) {
        ssize_t bytes = libssh2_channel_read_stderr(channel, buffer, sizeof(buffer));
        if (bytes > 0) {
            std::string chunk(buffer, buffer + bytes);
            stderrText += chunk;
            if (onOutput) onOutput(chunk);
            continue;
        }
        if (bytes == LIBSSH2_ERROR_EAGAIN) continue;
        break;
    }

    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);

    int exitCode = libssh2_channel_get_exit_status(channel);
    libssh2_channel_free(channel);

    while (!result.output.empty() && (result.output.back() == '\n' || result.output.back() == '\r')) {
        result.output.pop_back();
    }
    while (!stderrText.empty() && (stderrText.back() == '\n' || stderrText.back() == '\r')) {
        stderrText.pop_back();
    }

    result.success = exitCode == 0;
    result.error = stderrText;
    if (!result.success && result.error.empty()) {
        result.error = "Command exited with code " + std::to_string(exitCode) + ".";
    }

    return result;
}
