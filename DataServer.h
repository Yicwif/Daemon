#ifndef _DATA_SERVER_H_
#define _DATA_SERVER_H_

class Server
{
public:
    Server();
    virtual ~Server();
    void Init();
    int Run(unsigned short uPort);
    int CreateSocket();
    int BindSocket();
    int ListenSocket();
    int AcceptSocket();
    int GetCmdType(const char* pCmd);
    int DealDIRcmd(const char* pCmd);
    int DealFILECmd(const char* pCmd);
    void Error_Handling(const char* sMessage);
    void IsGZFile(const char *pFileName);
private:
    void SetPort(unsigned short uPort){ m_uPort = uPort; };
    int m_serv_sock;
    int m_clnt_sock;
    struct sockaddr_in m_serv_addr;
    struct sockaddr_in m_clnt_addr;
    socklen_t m_clnt_addr_size;
    FILE * m_pFile;
    unsigned short m_uPort;
    bool m_bIsGZFile;
};
#endif
