#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <new>
#include "DataServer.h"
#include "DataClient.h"
using namespace std;

int main(int argc, char** argv)
{
    int iRet = 0;
    if (argc > 2) {
        printf("This is data Demander!");
        Client *pClient = new(std::nothrow) Client();
        if(pClient == NULL)
        {
            iRet = -1;
            return iRet;
        }
        cout<<argv[1]<<" "<<atoi(argv[2])<<endl;
        pClient->Run(argv[1], atoi(argv[2]), (argc==3)?  NULL : argv[3] );
    }
    if (argc == 2)
    {
        printf("This is data Provider!");
        Server *pServer = new(nothrow) Server();
        if(pServer == NULL)
        {
            iRet = -1;
            return iRet;
        }
        pServer->Run(atoi(argv[1]));
    }
    return iRet;
}
