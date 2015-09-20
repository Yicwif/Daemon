#ifndef _DATA_CLIENT_H_
#define _DATA_CLIENT_H_

class Client
{
public:
    Client();
    virtual ~Client();
    void Init();
    int Run(const char* pIP, unsigned short uPort, const char * pFileName = NULL);
    int CreateSocket();
    int ConnectSocket();
    int RequireDir();
    int RequireFile();
    void IsGZFile();
    void Error_Handling(const char* sMessage);
private:
    void SetPort(unsigned short uPort){ m_uPort = uPort; };
    int m_sock;
    struct sockaddr_in m_serv_addr;
    FILE * m_pFile;
    unsigned short m_uPort;
    char  m_sFilename[256];
    char  m_sIPAddress[32];
    int m_RequireType;
    bool m_bIsGZFile;

};
#endif
