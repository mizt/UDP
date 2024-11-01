#import <sys/types.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>
#import <unistd.h>

namespace UDP {
    
    typedef struct message {
        char *format;
        char *marker;
        char *buffer;
        uint32_t len;
    } message;
    
    class Receive {
            
        private:
            
            const int udp = socket(AF_INET,SOCK_DGRAM,0);
            char buffer[2048];
            
            int parse(message *o, char *b, const int len) {
                int i = 0;
                while(b[i]!='\0') i++;
                while(b[i]!=',')  i++;
                if(i>=len) return -1;
                o->format = b+i+1;
                while(i<len&&b[i]!='\0') i++;
                if(i==len) return -2;
                i = (i+4)&~0x3;
                o->marker = b+i;
                o->buffer = b;
                o->len = len;
                return 0;
            }
            
        public:
            
            virtual void receive(const char *key,int value) {
                printf("%s,%d\n",key,value);
            }
            
            virtual void receive(const char *key,const char *value) {
                printf("%s,%s\n",key,value);
            }
            
            void update() {
                if(this->udp<=0) return;
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(this->udp,&readSet);
                struct timeval timeout = {1, 0};
                if(select(this->udp+1, &readSet, NULL, NULL, &timeout) > 0) {
                    struct sockaddr sa;
                    socklen_t sa_len = sizeof(struct sockaddr_in);
                    int len = 0;
                    while((len=(int)recvfrom(this->udp,buffer,sizeof(buffer),0,&sa,&sa_len))>0) {
                        if(((*(const int64_t *)buffer)!=htonll(0x2362756E646C6500L))) { // not #bundle
                            message osc;
                            if(this->parse(&osc,buffer,len)==0) {
                                
                                switch (osc.format[0]) {
                                    
                                    case 'i':
                                        receive(osc.buffer,ntohl(*((uint32_t *)osc.marker)));
                                        osc.marker+=4;
                                        break;
                                    case 's': 
                                        int size = (int) strlen(osc.marker);
                                        if(osc.marker+size<osc.buffer+osc.len) {
                                            const char *s = osc.marker;
                                            receive(osc.buffer,s);
                                            osc.marker+=((size+4)&~0x3);
                                        }
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            
            Receive(unsigned short port) {
                fcntl(this->udp,F_SETFL,O_NONBLOCK); // non-blocking
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = INADDR_ANY;
                addr.sin_port = htons(port);
                bind(this->udp,(struct sockaddr *)&addr,sizeof(struct sockaddr_in));
            }
            
            ~Receive() {
                close(this->udp);
            }
    };
    
    class Send {
        
        private:
            
            struct sockaddr_in addr;
            const int udp = socket(AF_INET,SOCK_DGRAM,0);
            char buffer[256];
        
        public:
        
            void send(const char *address, int int32) {
                if(udp) {
                    int size = (int)strlen(address);
                    int bits = ((size+4)&~0x3);
                    long total = bits+4+4; // address + type + int32
                    if(total>=1024) return;
                    char *p = buffer;
                    for(int k=0; k<bits; k++)  *p++ = (size>k)?address[k]:0;
                    *p++ = ','; *p++ = 'i'; *p++ = 0; *p++ = 0;
                    for(int k=0; k<4; k++) {
                        int shift = ((4-1-k)<<3);
                        *p++ = (int32>>shift)&0xFF;;
                    }
                    sendto(udp,buffer,total,0,(struct sockaddr *)&addr,sizeof(addr));
                }
            }
            
            void send(const char *address, const char *message) {
                if(udp) {
                    int addressSize = (int)strlen(address);
                    int addressBits = ((addressSize+4)&~0x3);
                    int messageSize = (int)strlen(message);
                    int messageBits = ((messageSize+4)&~0x3);
                    long total = addressBits+messageBits+4;
                    if(total>=1024) return;
                    char *p = buffer;
                    for(int k=0; k<addressBits; k++)  *p++ = (addressSize>k)?address[k]:0;
                    *p++ = ','; *p++ = 's'; *p++ = 0; *p++ = 0;
                    for(int k=0; k<messageBits*4; k++) *p++ = (messageSize>k)?message[k]:0;
                    sendto(udp,buffer,total,0,(struct sockaddr *)&addr,sizeof(addr));
                }
            }
            
            Send(const char *ip, int port) {
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = inet_addr(ip);
                addr.sin_port = htons(port);
            }
            
            ~Send() {
                close(this->udp);
            }
    };
}