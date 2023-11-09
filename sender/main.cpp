#pragma comment(lib,"Ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdio>
#include<ctime>
#include <iostream>
#include"definations.h"
#include"FileReader.h"

int revstate = DISCONN;
SOCKET socketClient,socketListener;

char revData[BUFSIZ]={};
bool judge = true;
char sendData[128];
char Filename[BUFSIZ]={};

auto* dataframe = (INFODGRAM*)malloc(sizeof(INFODGRAM));
clock_t begin,end;

int Time_Out = 3000;

void setstate(int);
bool handShaking(SOCKADDR_IN);
void udpTransporting(FILE*,FileReader,SOCKADDR_IN);
bool handwaving(SOCKADDR_IN);
u_short checksum(u_short*,int);

int main(){
    FileReader fh{};
    int length;

    WORD Version = MAKEWORD(2, 2);;
    WSADATA wsaData;
    if (WSAStartup(Version, &wsaData) != 0) {
        printf("WSAStartup failed with error!");
        return 1;
    }

    socketClient=socket(AF_INET,SOCK_DGRAM,0);
    socketListener=socket(AF_INET,SOCK_DGRAM,0);

    SOCKADDR_IN addrServ;
    addrServ.sin_addr.S_un.S_addr=inet_addr("127.0.0.1");
    addrServ.sin_family=AF_INET;
    addrServ.sin_port=htons(8000);

    SOCKADDR_IN Csin;
    Csin.sin_addr.S_un.S_addr=inet_addr("127.0.0.1");
    Csin.sin_family=AF_INET;
    Csin.sin_port=htons(8001);

    if(bind(socketListener,(SOCKADDR*)&Csin,sizeof(SOCKADDR))){
        printf("Bind error!\n");
        return 0;
    }

    //设置连接超时为3s
    setsockopt(socketListener, SOL_SOCKET, SO_RCVTIMEO, (char *)&Time_Out, sizeof(int));

    while(true) {
        FILE *f = fh.selectFile();
        if(revstate == DISCONN){
            if(!handShaking(addrServ)){
                //连接不上，回归非连接状态
                continue;
            }
            udpTransporting(f,fh,addrServ);
            Sleep(100);
            if(handwaving(addrServ)){
                setstate(DISCONN);
            }
            else{
                setstate(DISCONN);
            }
        }
        else{
            udpTransporting(f,fh,addrServ);
            Sleep(100);
            if(handwaving(addrServ)){
                setstate(DISCONN);
            }
            else{
                setstate(DISCONN);
            }
        }
    }
    closesocket(socketClient);
    WSACleanup();
    return 0;
}


//状态转换
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

bool handShaking(SOCKADDR_IN addrServ){
    dataframe->setSYN();
    //请求握手，发送SYN
    sendto(socketClient, (char*) dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
    dataframe->flush();
    //接收来自对方的SYN
    recv(socketListener, revData, 4, 0);
    if (((INFODGRAM*)revData)->getSYN() && ((INFODGRAM*)revData)->getACK()) {
        //发送ACK，进行状态切换，连接成功
        dataframe->setACK();
        sendto(socketClient, (char*)dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
        dataframe->flush();
        setstate(CONN);
        printf("Connected!\n");
        return true;
    }
    else {
        //超时或返回错误信息，连接失败
        printf("Receiver not on line!\n");
        return false;
    }
}

void udpTransporting(FILE* f,FileReader fh,SOCKADDR_IN addrServ){
    //调整面向重传的延迟机制
    Time_Out = 50;
    setsockopt(socketListener, SOL_SOCKET, SO_RCVTIMEO, (char *)&Time_Out, sizeof(int));
    //发送启动报文
    dataframe->setBegin();
    sendto(socketClient, (char*)dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
    dataframe->flush();
    //发送文件名称
    strcpy(Filename, fh.getFileName());
    sendto(socketClient, Filename, 256, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
    //发送文件内容,附加奇偶校验
    short times;
    bool marker = true;
    while (fread(sendData, 1, 256, f) > 0 ) {
        //重传次数
        times = 0;
        strcpy(revData,"NAK\n");
        while(times <= 2) {
            strcpy(revData,"NAK\n");
            //计算校验和
            u_short cksum = checksum((u_short *) sendData, 128);
            //包装协议报文
            auto *data = (INFODGRAM*)malloc(sizeof(INFODGRAM) + 256*sizeof(char));
            data->setNum(marker);
            data->cksum = cksum;
            memcpy(data->sendData,sendData,sizeof(char)*256);
            //发送报文
            sendto(socketClient, (char *) data, 260, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
            free(data);
            begin = clock();
            recv(socketListener, revData, BUFSIZ, 0);
            end = clock();
            //正常收到且为超时
            if(((INFODGRAM*)revData)->getACK() && end - begin<50){
                marker = !marker;
                break;  //进行下一次取内容和传递
            } else if(((INFODGRAM*)revData)->getNAK()){
                times++;
                continue; //继续传输
            }
        }
    }
    //发送结束报文
    dataframe->setEnd();
    sendto(socketClient, (char*)dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
    dataframe->flush();
    //传输完成
    printf("Transported successfully!\n");
}

bool handwaving(SOCKADDR_IN addrServ){
    //改回来，2秒延时
    Time_Out = 2000;
    setsockopt(socketListener, SOL_SOCKET, SO_RCVTIMEO, (char *)&Time_Out, sizeof(int));
    dataframe->setFIN();
    //请求握手，发送FIN
    sendto(socketClient, (char*)dataframe, 4, 0, (SOCKADDR *) &addrServ, sizeof(SOCKADDR));
    dataframe->flush();
    //接收来自对方的ACK
    recv(socketListener, revData, 4, 0);
    if(((INFODGRAM*)revData)->getACK()){
        printf("Disconnected!\n");
        return true;
    }
    else{
        printf("Out of time,Disconnected!\n");
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
