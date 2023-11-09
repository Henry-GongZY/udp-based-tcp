#define DISCONN 0
#define WAIT 1
#define CONN 2

struct INFODGRAM{
    u_char infoh{}; //四个校验位，一个编码位，一个缓冲区剩余大小最高位
    u_char infol{}; //缓冲区大小低位
    u_short cksum;    //校验和
    char sendData[0]; //变长数据段

    INFODGRAM(){
        cksum = 0;
    }
    void setACK(){
        infoh = infoh | 0x80;
    }
    void setNAK(){
        infoh = infoh | 0x40;
    }
    void setSYN(){
        infoh = infoh | 0x20;
    }
    void setFIN(){
        infoh = infoh | 0x10;
    }
    void setBegin(){
        infoh = infoh | 0x08;
    }
    void setEnd(){
        infoh = infoh | 0x04;
    }
    void setNum(bool truth){
        if(truth) {
            infoh = infoh | 0x02;
        }
        else{
            infoh = infoh & 0xfd;
        }
    }
    bool getACK(){
        return infoh>>7;
    }
    bool getNAK(){
        return (infoh>>6) & 0x01;
    }
    bool getSYN(){
        return (infoh>>5) & 0x01;
    }
    bool getFIN(){
        return (infoh>>4) & 0x01;
    }
    bool getBegin(){
        return (infoh>>3) & 0x01;
    }
    bool getEnd(){
        return (infoh>>2) & 0x01;
    }
    bool getNum(){
        return infoh & 0x02;
    }
    int getLength(){
        int a=0;
        if((infoh & 0x1) == true){
            a+=256;
            int k = 1;
            u_char temp = infol;
            for(int i =0;i<=7;i++){
                a+=(int)(temp & 0x1)*k;
                temp = temp>>1;
                k*=2;
            }
            return a;
        }else{
            int k = 1;
            for(int i =0;i<=7;i++){
                a+=(infol & 0x1)*k;
                infol = infol>>1;
                k*=2;
            }
            return a;
        }
    }
    void setLength(int a){
        infoh = infoh & 0xfe;
        infol = infol & 0;
        if(a==512){
            setLength(511);
        }else if(a>=256){
            a -= 256;
            infoh = infoh | 0x01;
            infol = infol | a;
        }else{
            infoh = infoh & 0xfe;
            infol = infol | a;
        }
    }
    void flush(){
        infoh = infoh & 0x00;
        infol = infol & 0x00;
    }
};