class Engine
{
public:
    static Engine& Get()
    {
        static Engine instance;
        static PacketParser parser;
        return instance;
    }

    void Run();
private:
    Engine() {}
    ~Engine() {}

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};