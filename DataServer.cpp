#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "DataServer.h"

#ifndef END_FLAG
#define END_FLAG "--END--"
#endif

#ifndef Max_Send_Size
#define Max_Send_Size 1024
#endif

Server::Server()
{
    Init();
}

Server::~Server()
{
    if (m_pFile != NULL)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }
}

void Server::Init()
{
    m_serv_sock = 0;
    m_clnt_sock = 0;
    memset(&m_serv_addr, 0 ,sizeof(m_serv_addr));
    memset(&m_clnt_addr, 0, sizeof(m_clnt_addr));
    m_clnt_addr_size = 0;
    m_pFile = NULL;
    m_uPort = 0;
    m_bIsGZFile = false;
}

int Server::Run(unsigned short uPort)
{
    int iRet = 0;

    SetPort(uPort);

    CreateSocket();

    BindSocket();

    ListenSocket();

    while(1)
    {
        AcceptSocket();
    }

    return iRet;
}

int Server::CreateSocket()
{
    int iRet = 0;
    m_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_serv_sock == -1) 
    {
        Error_Handling("socket() error\n");
    }
    return iRet;
}

int Server::BindSocket()
{
    int iRet = 0;
    m_serv_addr.sin_family = AF_INET;
    m_serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_serv_addr.sin_port = htons(m_uPort);
    if( bind(m_serv_sock,(struct sockaddr*)&m_serv_addr, sizeof(m_serv_addr)) < 0 ) {
        Error_Handling("bind() error\n");
    }
    return iRet;
}

int Server::ListenSocket()
{
    int iRet = 0;
    if ( listen(m_serv_sock, 16) == -1)
    {
        Error_Handling("lisen() error");
    }
    return iRet;
}

int Server::AcceptSocket()
{
    int iRet = 0;
    int iRecvLen = 0;
    int iSendLen = 0;
    char sRecvMsg[512] = {0};
    m_clnt_addr_size = sizeof(m_clnt_addr);
    m_clnt_sock = accept(m_serv_sock,(struct sockaddr*)&m_clnt_addr,&m_clnt_addr_size);
    if ( m_clnt_sock == -1) {
        Error_Handling("accept() error");
    }
    iRecvLen = recv(m_clnt_sock, sRecvMsg, sizeof(sRecvMsg), 0);
    printf("iRecvLen:%d\n", iRecvLen);
    if (iRecvLen < 0 )
    {
        printf("iRecvLen:%d\n", iRecvLen);
        iRet = -1;
        return iRet;
    }

    int iCmdType = GetCmdType(sRecvMsg);
    if (iCmdType == 1)         //DIR
    {
        DealDIRcmd(sRecvMsg);
    }
    else if (iCmdType == 2)     //FILE
    {
        DealFILECmd(sRecvMsg);
    }
    close(m_clnt_sock);
    return iRet;
}

int Server::DealDIRcmd(const char* pCmd)
{
    int iRet = 0;
    char sPathFileName[256] = {0};
    char sSendBuf[512] = {0};
    DIR * pDir = NULL;
    dirent *pDirent = 0;
    struct stat StatBuf;
    if ((pDir = opendir("./")))
    {
        while ((pDirent = readdir(pDir)))
        {
            if (0 == strcmp(pDirent->d_name, ".") || 0 == strcmp(pDirent->d_name, ".."))
            {
                continue;
            }
            memccpy(sPathFileName, pDirent->d_name, 0, sizeof(sPathFileName) - 1);
            sPathFileName[ sizeof(sPathFileName) - 1] = 0;

            if (stat(sPathFileName, &StatBuf) != 0)
            {
                continue;
            }

            if ( (StatBuf.st_mode & S_IFDIR) == S_IFDIR)//if is dir
            {
                strcat(sPathFileName, "/");
                sPathFileName[ sizeof(sPathFileName) - 1] = 0;
                std::cout<<"Send dir name: "<<sPathFileName<<std::endl;
            }
            else
            {   
                std::cout<<"Send file name: "<<sPathFileName
                    <<"  size:"<<StatBuf.st_size<<std::endl;
            }
            snprintf(sSendBuf, sizeof(sSendBuf), "%s   %ld\n", sPathFileName,StatBuf.st_size);
            send(m_clnt_sock, sSendBuf, strlen(sSendBuf), 0);//send to client
        }
    }
    closedir(pDir);
    pDir = NULL;
    snprintf(sSendBuf, sizeof(sSendBuf), END_FLAG"\n");
    send(m_clnt_sock, sSendBuf, strlen(sSendBuf), 0);//send end flag to client
    return iRet;
}

int Server::DealFILECmd(const char* pCmd)
{
    int iRet = 0;
    if (pCmd == NULL)
    {
        std::cout<<"Which file to get?"<<std::endl;
        return iRet;
    }

    int iCmdLength = strlen(pCmd);// FILE ####
    char sSendBuf[Max_Send_Size] = {0};
    char sRecvBegin[6] = {0};
    char sFileName[512] = {0};
    long lFileSize = 0;
    long lModSize = 0;
    long lBlockNum = 0;
    int iReadlen = 0;
    snprintf(sFileName, sizeof(sFileName), "./%s", pCmd+5);
    sFileName[sizeof(sFileName) - 1] = 0;
    if (access(sFileName, F_OK) != 0)//not exist
    {
        iRet = -1;
        std::cout<<"No file "<<sFileName<<std::endl;
        snprintf(sSendBuf, sizeof(sSendBuf), "NO");
        send(m_clnt_sock, sSendBuf, 2, 0);
        return iRet;
    }
    if(m_pFile != NULL)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }

    IsGZFile(sFileName);
    if(!m_bIsGZFile)      //not .gz file, need to be gziped.
    {
        char pGzip[512] = {0};
        snprintf(pGzip, sizeof(pGzip), "gzip -c %s > %s.gz", sFileName, sFileName);
        iRet = system(pGzip);

        strncat(sFileName, ".gz", 3);
    }
    if (access(sFileName, F_OK) != 0)//not exist
    {
        iRet = -1;
        std::cout<<"No file "<<sFileName<<std::endl;
        snprintf(sSendBuf, sizeof(sSendBuf), "NO");
        send(m_clnt_sock, sSendBuf, 2, 0);
        return iRet;
    }

    m_pFile = fopen(sFileName, "rb+");
    if (m_pFile  == NULL)
    {
        iRet = -1;
        return iRet;
    }
    fseek(m_pFile, 0, SEEK_END);
    lFileSize= ftell(m_pFile);
    lBlockNum = lFileSize  /(long)Max_Send_Size;
    lModSize = lFileSize % (long)Max_Send_Size;
    fseek(m_pFile, 0, SEEK_SET);

    snprintf(sSendBuf, sizeof(sSendBuf), "%ld", lFileSize);
    send(m_clnt_sock, sSendBuf, sizeof(sSendBuf), 0);
    recv(m_clnt_sock, sRecvBegin, sizeof(sRecvBegin), 0);
    if(strncmp(sRecvBegin, "BEGIN", 5) == 0)
    {
        long lcount = 0;
        int iSendSize = 0;
        std::cout<<"lBlockNum:"<<lBlockNum <<" lModSize:"<<lModSize<<std::endl;
        for (long iCurBlock = 0; iCurBlock < lBlockNum; iCurBlock++)
        {
            iReadlen= fread(sSendBuf, 1, Max_Send_Size, m_pFile);
            if (iReadlen != 0)
            {
                iSendSize = send(m_clnt_sock, sSendBuf, iReadlen, 0);
                lcount++;
                if (iSendSize != (int)Max_Send_Size)
                {
                    std::cout<<"lCount:"<<lcount<<" iSendSize:"<<iSendSize<<std::endl;
                }
            }

        }
        std::cout<<"sendtime: "<<lcount<<std::endl;
        if (lModSize != 0)
        {
            iReadlen= fread(sSendBuf, 1, lModSize, m_pFile);
            send(m_clnt_sock, sSendBuf, iReadlen, 0);	
        }
    }
    fclose(m_pFile);
    m_pFile = NULL;
    if(!m_bIsGZFile)
    {
        unlink(sFileName);
    }

    return iRet;
}

int Server::GetCmdType(const char* pCmd)
{
    int iRetCmdType = -1;
    if (strlen(pCmd) > 0)
        iRetCmdType = 0;

    if (iRetCmdType == 0 && strncmp(pCmd, "DIR", 3) == 0)
    {
        iRetCmdType = 1;
    }
    else if(iRetCmdType == 0 && strncmp(pCmd, "FILE", 4) == 0)
    {
        iRetCmdType = 2;
    }
    else
    {
        iRetCmdType = -1;
    }
    return iRetCmdType;
}

void Server::IsGZFile(const char *pFileName)
{
    if(strncmp(pFileName+strlen(pFileName)-3, ".gz", 3) == 0)
    {
        m_bIsGZFile = true;
    }
    else
    {
        m_bIsGZFile = false;
    }
}

void Server::Error_Handling(const char* sMessage) 
{
    fputs(sMessage, stderr);
    fputc('\n', stderr);
    printf("Error!\n");
    exit(1);
}











