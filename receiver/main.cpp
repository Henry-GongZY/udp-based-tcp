#pragma comment(lib,"Ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdio>
#include <ctime>
#include <iostream>
#include "FileReader.h"
#include "definations.h"

int revstate = DISCONN;
SOCKET socketListener,socketClient;

FileReader fh{};
FILE *f;
SOCKADDR_IN addrClient;
int length=sizeof(SOCKADDR);
int revlen=0;
char recvBuf[BUFSIZ]={};
char Filename[BUFSIZ]={};
char ClientAddr[BUFSIZ]={};
char FromName[BUFSIZ]={};
auto* dataframe = (INFODGRAM*)malloc(sizeof(INFODGRAM));
clock_t begin,end;

bool connect(SOCKADDR_IN);
void setstate(int);
bool handwaving(SOCKADDR_IN);
void udpTransporting(SOCKADDR_IN);
u_short checksum(u_short *,int);

int main()
{
    WSADATA wsaData;
    //创建WSA
    WORD Version=MAKEWORD(2,2);
    if (WSAStartup(Version,&wsaData)!=0){
        printf("WSAStartup failed with error!\n");
        return -1;
    }

    //创建SOCKET
    socketClient=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    socketListener=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

    //SOCKET绑定接受地址
    SOCKADDR_IN addrServ;
    addrServ.sin_family=AF_INET;
    addrServ.sin_addr.S_un.S_addr=inet_addr("127.0.0.1");
    addrServ.sin_port=htons(8000);

    //SOCKET发送地址
    SOCKADDR_IN addrServ2;
    addrServ2.sin_family=AF_INET;
    addrServ2.sin_addr.S_un.S_addr=inet_addr("127.0.0.1");
    addrServ2.sin_port=htons(8001);

    if(bind(socketClient,(SOCKADDR*)&addrServ,sizeof(SOCKADDR))){
        printf("Bind error!\n");
        return 0;
    }

    while (true){
        f = nullptr;
        //非联通状态
        if(revstate == DISCONN) {
            if(connect(addrServ2)){
                udpTransporting(addrServ2);
                Sleep(100);
                if(handwaving(addrServ2)){
                    printf("Disconnected!\n");
                    setstate(DISCONN);
                }
                else{
                    printf("Handwaving error!Disconnected!\n");
                    setstate(DISCONN);
                }
            }
            else{
                printf("Quit!");
                closesocket(socketClient);
                closesocket(socketListener);
                WSACleanup();
                return 0;
            }
        }
        else{
            udpTransporting(addrServ2);
            Sleep(100);
            if(handwaving(addrServ2)){
                printf("Disconnected!\n");
                setstate(DISCONN);
            }
            else{
                printf("Handwaving error!Disconnected!\n");
                setstate(DISCONN);
            }
        }
    }
    closesocket(socketClient);
    closesocket(socketListener);
    WSACleanup();
    return 0;
}


void setstate(int state){
    switch (state){
        case DISCONN:
            revstate = DISCONN;
            return;
        case WAIT:
            revstate = WAIT;
            return;
        case CONN:
            revstate = CONN;
            return;
        default:
            return;
    }
}

bool connect(SOCKADDR_IN addrServ){
    int Time_Out = 50000;
    //设置接受待机时间为50秒
    setsockopt(socketClient,SOL_SOCKET, SO_RCVTIMEO, (char *)&Time_Out, sizeof(int));
    while(true) {
        //接收SYN
        recvfrom(socketClient,recvBuf,4,0,(SOCKADDR*)&addrClient,&length);
        if (((INFODGRAM*)recvBuf)->getSYN()) {
            dataframe->setSYN();
            dataframe->setACK();
            sendto(socketListener, (char*)dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
            dataframe->flush();
            recvfrom(socketClient, recvBuf, 4, 0, (SOCKADDR *) &addrClient, &length);
            if (((INFODGRAM*)recvBuf)->getACK()) {
                printf("Connected!\n");
                setstate(CONN);
                return true;
            } else {
                printf("Connection error!\n");
                return false;
            }
        } else if (((INFODGRAM*)recvBuf)->getACK() || (strlen(recvBuf) == 0)) {
            printf("Time over limit!\n");
            return false;
        }
    }
}

void udpTransporting(SOCKADDR_IN addrServ){
    while(true) {
        //启动报文接收
        recv(socketClient, recvBuf, 256, 0);
        //收到的报文是启动报文则开始进行存储，写入文件
        if (((INFODGRAM*)recvBuf)->getBegin()) {
            //开启文件保存到特定文件夹
            recvfrom(socketClient, recvBuf, 256, 0, (SOCKADDR *) &addrClient, &length);
            strcpy(ClientAddr, inet_ntoa(addrClient.sin_addr));
            strcpy(FromName, recvBuf);
            fh.createDir(ClientAddr);
            strcpy(Filename, ClientAddr);
            strcat(Filename, "\\");
            strcat(Filename, recvBuf);
            f = fh.createFile(Filename);
            break;
        }
        else if(strlen(recvBuf) == 0){
            printf("File transporting error! Disconnected!\n");
            setstate(DISCONN);
            return;
        }
    }
    int sum=0;
    bool marker = true;
    while((revlen=recvfrom(socketClient,recvBuf,260,0,(SOCKADDR*)&addrClient,&length))>0)
    {
        //结束报文则结束文件传输
        if (((INFODGRAM*)recvBuf)->getEnd())
        {
            printf("File:%s successfully transported!\n",FromName);
            fclose(f);
            break;
        }
        //重新进行校验和计算
        u_short k = checksum((u_short*)(((INFODGRAM*)recvBuf)->sendData),128);
        if(k == ((INFODGRAM*)recvBuf)->cksum && ((INFODGRAM*)recvBuf)->getNum() == marker) { //校验正确
            //回送接受报文
            dataframe->setACK();
            sendto(socketListener,(char*)dataframe, 4, 0,(SOCKADDR *) &addrServ, sizeof(SOCKADDR));
            dataframe->flush();
            //将发送的数据写入文件
            fwrite(((INFODGRAM *) recvBuf)->sendData, 1, 256, f);
            marker = !marker;
        }
        else{
            dataframe->setNAK();
            sendto(socketListener,(char*)dataframe, 4, 0,(SOCKADDR *) &addrServ, sizeof(SOCKADDR));
            dataframe->flush();
        }
    }
    //未出现结束报文或连接断开
    if (revlen<0||!((INFODGRAM*)recvBuf)->getEnd())
    {
        printf("IP:%s File:%s Disconnected while transporting!\n",addrClient,FromName);
        fclose(f);
        remove(Filename);
    }

}

bool handwaving(SOCKADDR_IN addrServ){
    int Time_Out = 10000;
    //FIN挥手接受时间上限为2秒，超时自动停机
    setsockopt(socketClient,SOL_SOCKET, SO_RCVTIMEO, (char *)&Time_Out, sizeof(int));
    //接受FIN
    recvfrom(socketClient,recvBuf,4,0,(SOCKADDR*)&addrClient,&length);
    if (((INFODGRAM *) recvBuf)->getFIN()) {
        //发送ACK
        dataframe->setACK();
        sendto(socketListener, (char *) dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
        dataframe->flush();
        return true;
    }
    else {
        printf("Out of time!");
        return false;
    }
}

u_short checksum(u_short *buf,int count)
{
    u_long sum = 0;
    while(count--)
    {
        sum += *buf++;
        if(sum&0xFFFF0000){
            sum&=0xFFFF;
            sum++;
        }
    }
    return ~(sum&0xFFFF);
}