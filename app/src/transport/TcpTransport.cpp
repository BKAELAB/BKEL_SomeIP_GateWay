#include "transport/TcpTransport.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

// 생성자 초기화
TcpTransport::TcpTransport() : fd_(-1), is_listen_(false) {}
TcpTransport::TcpTransport(int fd) : fd_(fd), is_listen_(false) {}
TcpTransport::~TcpTransport() { Close(); }

// 이동 생성자 초기화
TcpTransport::TcpTransport(TcpTransport&& other) noexcept {
    fd_ = other.fd_;
    is_listen_ = other.is_listen_;
    other.fd_ = -1; // 오타 수정
    other.is_listen_ = false;
}

TcpTransport& TcpTransport::operator=(TcpTransport&& other) noexcept {
    if (this == &other) return *this;
    Close();
    fd_ = other.fd_;
    is_listen_ = other.is_listen_;
    other.fd_ = -1;
    other.is_listen_ = false;
    return *this;
}

//bool에서 void로
void TcpTransport::Close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    is_listen_ = false;
}

// 소켓옵션 설정(port 번호 재사용)
bool TcpTransport::SetReuseAddr(bool on) {
    if (fd_ < 0) return false;
    int v = on ? 1: 0;
    return ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) == 0;
}

// Server
bool TcpTransport::Listen(const std::string& bind_ip, uint16_t port, int backlog)
{
    Close();

    // 소켓 생성
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) return false;

    // 포트번호 재사용 옵션 ON/OFF
    SetReuseAddr(true);

    // bind() 구조체 값 초기화
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (bind_ip.empty() || bind_ip == "0.0.0.0") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else 
    {
        if(::inet_pton(AF_INET, bind_ip.c_str(), &addr.sin_addr) != 1) {
            Close();
            return false;
        }
    }

    if (::bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Close();
        return false;
    }

    if (::listen(fd_, backlog) < 0) {
        Close();
        return false;
    }
    
    // 연결용 소켓
    is_listen_ = true;
    return true;

}

std::unique_ptr<TcpTransport> TcpTransport::Accept(int timeout)
{
    if (fd_ < 0 || !is_listen_) return nullptr;

    
    int cfd = ::accept(fd_, nullptr, nullptr);
    if (cfd < 0) return nullptr;

    auto conn = std::make_unique<TcpTransport>(cfd);
    return conn;
}


// Clinet
bool TcpTransport::Connect(const std::string& ip, uint16_t port) {
    Close();

    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        Close();
        return false;
    }

    if (::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        Close();
        return false;
    }

    is_listen_ = false; // 클라이언트는 listen 상태가 아님
    return true;
}

// 공통 송수신
int TcpTransport::SendData(const uint8_t* data, size_t len) {
    if (fd_ < 0 || is_listen_) return -1;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd_, data + sent, len - sent, 0);
        if (n > 0) {
            sent += (size_t)n;
            continue;
        }
        if (n == 0) return (int)sent;
        if (errno == EINTR) continue;
        return -1;
    }
    return (int)sent;
}

int TcpTransport::RecvData(uint8_t* buf, size_t len) {
    if (fd_ < 0 || is_listen_) return -1;
    while (true) {
        ssize_t n = ::recv(fd_, buf, len, 0); // 세미콜론 추가
        if (n >= 0) return (int)n;
        if (errno == EINTR) continue;
        return -1;
    }
}
