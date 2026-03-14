class Engine 
{
private:
    Engine();
    ~Engine();
    
    static Engine m_Inst;
public:
    
    static Engine& Get_Engine_Inst() 
    {
        return m_Inst;    
    }

};