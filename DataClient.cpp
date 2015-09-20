#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include "DataClient.h"

#ifndef END_FLAG
#define END_FLAG "--END--"
#endif

#ifndef Max_Send_Size
#define Max_Send_Size 1024
#endif

#ifndef Max_FileName_Size
#define Max_FileName_Size 256
#endif



Client::Client()
{
    Init();
}

Client::~Client()
{
}

void Client::Init()
{
    m_sock = 0;
    memset(&m_serv_addr, 0, sizeof(m_serv_addr));
    m_pFile = NULL;
    memset(m_sFilename, 0, sizeof(m_sFilename));
    memset(m_sIPAddress, 0, sizeof(m_sIPAddress));
    m_RequireType = 0;
    m_bIsGZFile = false;
}

int Client::Run(const char* pIP, unsigned short uPort, const char * pFileName)
{
    int iRet = 0;
    if (pIP == NULL)
    {
        iRet = -1;
        return iRet;
    }
    memccpy(m_sIPAddress, pIP, 0, sizeof(m_sIPAddress) - 1);
    m_sIPAddress[sizeof(m_sIPAddress) - 1] = 0;
    m_uPort = uPort;
    if (pFileName != NULL){
        m_RequireType = 2; //File
        memccpy(m_sFilename, pFileName, 0, sizeof(m_sFilename) - 1);
        m_sFilename[sizeof(m_sFilename) - 1] = 0;
    }
    else
    {
        m_RequireType = 1; //DIR
    }

    CreateSocket();
    ConnectSocket();
    return iRet;
}

int Client::CreateSocket()
{
    int iRet = 0;
    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_sock < 0 )
    {
        Error_Handling("CreateSocket ERROR!\n");
        iRet = -1;
    }
    return iRet;
}

int Client::ConnectSocket()
{
    int iRet = 0;
    m_serv_addr.sin_family = AF_INET;
    m_serv_addr.sin_addr.s_addr = inet_addr(m_sIPAddress);
    m_serv_addr.sin_port = htons(m_uPort);

    if( connect(m_sock,(struct sockaddr*)&m_serv_addr,sizeof(m_serv_addr)) == -1) {
        Error_Handling("connect() error\n");
    }

    if(m_RequireType == 1)
    {
        RequireDir();
    }
    else if (m_RequireType == 2)
    {
        RequireFile();
    }
    else
        iRet = -1;
    return iRet;
}

int Client::RequireDir()
{
    int iRet = 0;
    int bflag = true;
    int iRecvSize = 0;
    char sSendbuf[512] = {0};
    char sRecvbuf[512] = {0};
    snprintf(sSendbuf, sizeof(sSendbuf), "DIR\n" );
    send(m_sock, sSendbuf, strlen(sSendbuf), 0);

    while(bflag)
    {
        iRecvSize = recv(m_sock, sRecvbuf, sizeof(sRecvbuf), 0);
        printf("%s", sRecvbuf);
        if(strncmp(sRecvbuf+iRecvSize-8, "--END--", 7) == 0)
        {
            bflag = false;
        }
    }
    close(m_sock);
    return iRet;
}

int Client::RequireFile()
{
    int iRet = 0;
    char sRequireCmd[Max_FileName_Size+5] = {0};
    char sRecvBuf[Max_Send_Size] = {0};
    int iRecvSize = 0;
    long lTotalRecvSize = 0;
    long lRemainSize = 0;
    long lFileSize = 0;
    long lModSize = 0;
    long lBlockNum = 0;
    int iReadlen = 0;

    snprintf(sRequireCmd, sizeof(sRequireCmd), "FILE %s", m_sFilename);
    send(m_sock, sRequireCmd, strlen(sRequireCmd), 0);

    recv(m_sock, sRecvBuf, Max_Send_Size, 0);
    printf("--%s\n", sRecvBuf);
    if(strncmp(sRecvBuf, "NO", 2) == 0)
    {
        printf("No file %s on Provider!\n", m_sFilename);
        iRet = -1;
        close(m_sock);
        return -1;
    }
    else
    {
        lFileSize = atol(sRecvBuf);  //get file size
    }


    IsGZFile();
    if (!m_bIsGZFile)
    {
        strncat(m_sFilename, ".gz", 3);
    }
    printf("%s size:%ld\n", m_sFilename,lFileSize);
    lBlockNum = lFileSize  /(long)Max_Send_Size;
    lModSize = lFileSize % (long)Max_Send_Size;
    m_pFile = fopen(m_sFilename, "w+");
    if (m_pFile == NULL)
    {
        iRet = -1;
        close(m_sock);
    }
    send(m_sock, "BEGIN", 5, 0);//ask server begin to send file
    printf("Recv BEGIN\n");

    lRemainSize = lFileSize;
    long lRecount = 0;
    do{
        if(lRemainSize > (long)Max_Send_Size)
        {
            iRecvSize = recv(m_sock, sRecvBuf, Max_Send_Size, 0);
        }
        else
        {
            iRecvSize = recv(m_sock, sRecvBuf, lRemainSize, 0);
        }
        lRecount++;
        if (iRecvSize != (int)Max_Send_Size)
        {
            //printf("lRecount:%ld, iRecvSize:%d, not 2048\n", lRecount, iRecvSize);
        }

        if(iRecvSize > 0)
        {
            lTotalRecvSize += iRecvSize;
            lRemainSize -= iRecvSize;
            fwrite(sRecvBuf, 1, iRecvSize, m_pFile);
        }

    }while(lRemainSize> 0);
    printf("lRecount:%ld\n",lRecount);

    fclose(m_pFile);
    m_pFile = NULL;

    if (!m_bIsGZFile)
    {
        char pGunzip[Max_FileName_Size] = {0};
        snprintf(pGunzip, sizeof(pGunzip), "gunzip %s", m_sFilename);
        iRet = system(pGunzip);
        printf("system function return %d\n", iRet);
        iRet = 0;
    }

    close(m_sock);

    return iRet;

}

void Client::IsGZFile()
{
    if (strncmp(m_sFilename+strlen(m_sFilename)-3, ".gz", 3) == 0)
    {
        m_bIsGZFile = true;
    }
    else
    {
        m_bIsGZFile = false;
    }
}

void Client::Error_Handling(const char* sMessage) 
{
    fputs(sMessage, stderr);
    fputc('\n', stderr);
    printf("Error!\n");
    exit(1);
}

