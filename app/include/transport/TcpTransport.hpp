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
    TcpTransport(TcpTransport&&) noexcept;
    TcpTransport& operator=(TcpTransport&&) noexcept;

    /* Server */
    // socket() -> bind() -> listen() -> accept -> send/recv -> close
    bool Listen(const std::string& bind_ip, uint16_t port, int backlog);
    void Accept();

    /* Client */
    
    //Send/Recv
    int SendData(const uint8_t* data, size_t len);
    int RecvData();

    void Close();

private:
    bool SetReuseAddr(bool on);

private:
    int fd_;
    int is_listen_;

}