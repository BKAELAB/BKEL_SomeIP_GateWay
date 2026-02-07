#include <string>      // std::string 사용을 위해
#include <memory>      // std::unique_ptr 사용을 위해
#include <cstdint>     // uint8_t 사용을 위해
#include <cstddef>     // size_t 사용을 위해 (일부 환경 필수)

class TcpTransport {
public:
    // 생성자
    TcpTransport();
    // accept(), 새로 연결되었을 경우
    explicit TcpTransport(int fd);

    /* Rule of Five */
    // 소멸자 
    ~TcpTransport();

    // 복사 생성자 금지
    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    // 이동 생성자
    TcpTransport(TcpTransport&& other) noexcept;
    TcpTransport& operator=(TcpTransport&& other) noexcept;

    /* Server */
    // socket() -> bind() -> listen() -> accept -> send/recv -> close
    bool Listen(const std::string& bind_ip, uint16_t port, int backlog);
    std::unique_ptr<TcpTransport> Accept(int timeout);

    /* Client */
    // Connect
    bool Connect(const std::string& ip, uint16_t port);

    // Send/Recv
    int SendData(const uint8_t* data, size_t len);
    int RecvData(uint8_t* buf, size_t len);

    void Close();

private:
    bool SetReuseAddr(bool on);

private:
    int fd_;
    bool is_listen_;
};
