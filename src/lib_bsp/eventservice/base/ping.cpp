#include <stdio.h>  
#include <sys/time.h>  
#include <netdb.h>  
#include <stdlib.h>  
#include <sys/socket.h>  
#include <arpa/inet.h>  
#include <string.h>  
#include <netinet/ip_icmp.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <signal.h>  
#include <math.h>
#include <net/if.h>
#include "ping.h"
#include "eventservice/log/log_client.h"

#define ICMP_DATA_LEN 56
#define ICMP_HEAD_LEN 8
#define ICMP_LEN  (ICMP_DATA_LEN + ICMP_HEAD_LEN)  
#define SEND_BUFFER_SIZE 128
#define RECV_BUFFER_SIZE 128
#define SEND_NUM 100
#define MAX_WAIT_TIME 3 

#define WAIT_TIME 5  

namespace chml {

char SendBuffer[SEND_BUFFER_SIZE];  
char RecvBuffer[RECV_BUFFER_SIZE];  
int nRecv = 0;
struct timeval FirstSendTime;
struct timeval LastRecvTime;  
double min = 0.0;  
double avg = 0.0;  
double max = 0.0;  
double mdev = 0.0;  
  
u_int16_t Compute_cksum(struct icmp *pIcmp) {  
  u_int16_t *data = (u_int16_t *)pIcmp;  
  int len = ICMP_LEN;  
  u_int32_t sum = 0;  
        
  while (len > 1)  
  {  
    sum += *data++;  
    len -= 2;  
  }  
  if (1 == len)  
  {  
    u_int16_t tmp = *data;  
    tmp &= 0xff00;  
    sum += tmp;  
  }  

  while (sum >> 16)  
    sum = (sum >> 16) + (sum & 0x0000ffff);  
  sum = ~sum;  
        
  return sum;  
}  
  
void SetICMP(u_int16_t seq) {  
  struct icmp *pIcmp;  
  struct timeval *pTime;  
  
  pIcmp = (struct icmp*)SendBuffer;  
      
  pIcmp->icmp_type = ICMP_ECHO;  
  pIcmp->icmp_code = 0;  
  pIcmp->icmp_cksum = 0;
  pIcmp->icmp_seq = seq;
  pIcmp->icmp_id = getpid();
  pTime = (struct timeval *)pIcmp->icmp_data;  
  gettimeofday(pTime, NULL);
  pIcmp->icmp_cksum = Compute_cksum(pIcmp);  
}  
  
int SendPacket(int sock_icmp, struct sockaddr_in *dest_addr, int nSend) {  
  SetICMP(nSend);
  if (sendto(sock_icmp, SendBuffer, ICMP_LEN, 0,  
      (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) < 0) {  
    perror("sendto");  
    return -1;
  }

  return 0;
}  
  
double GetRtt(struct timeval *RecvTime, struct timeval *SendTime) {  
  struct timeval sub = *RecvTime;  
  
  if ((sub.tv_usec -= SendTime->tv_usec) < 0) {  
    --(sub.tv_sec);
    sub.tv_usec += 1000000;  
  }  
  sub.tv_sec -= SendTime->tv_sec;  
      
  return sub.tv_sec * 1000.0 + sub.tv_usec / 1000.0;
}  
  
int unpack(struct timeval *RecvTime) {  
  struct ip *Ip = (struct ip *)RecvBuffer;  
  struct icmp *Icmp;  
  int ipHeadLen;  
  double rtt;  
  
  ipHeadLen = Ip->ip_hl << 2;
  Icmp = (struct icmp *)(RecvBuffer + ipHeadLen);  
  
  if ((Icmp->icmp_type == ICMP_ECHOREPLY) && Icmp->icmp_id == getpid()) {    
    return 0;  
  }  
          
  return -1;  
}
  
int RecvePacket(int sock_icmp, struct sockaddr_in *dest_addr) {  
  int RecvBytes = 0;  
  socklen_t addrlen = sizeof(struct sockaddr_in);

  fd_set  readfds;

  int try_time = 2;

  while(try_time > 0) {
    FD_ZERO(&readfds);
    FD_SET(sock_icmp, &readfds);
    struct timeval TimeOut = {3, 0};

    int ret = select(sock_icmp+1, &readfds,NULL,NULL,&TimeOut);

    if(ret > 0) {
      RecvBytes = recvfrom(sock_icmp, RecvBuffer, RECV_BUFFER_SIZE,  
                           0, (struct sockaddr *)dest_addr, &addrlen);

    if (unpack(NULL) == 0)  
      return 0;
    }

    try_time--;
  }

  return -1;
} 

Ping::Ping() {
}

Ping::~Ping() {
}

bool Ping::ping(char *dev, char *ip) {
  if(ip == NULL)
    return false;

  int sock_icmp;
  struct hostent *pHost;  
  struct protoent *protocol;  
  struct sockaddr_in dest_addr;

  in_addr_t inaddr;
  
  if ((protocol = getprotobyname("icmp")) == NULL) {  
    perror("getprotobyname");  
    return false;
  } 

  if ((sock_icmp = socket(PF_INET, SOCK_RAW, protocol->p_proto/*IPPROTO_ICMP*/)) < 0) { 
    perror("socket");
    return false;
  }

  if(dev) {
    struct ifreq ifr;

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if(setsockopt(sock_icmp, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) != 0)
      goto PING_ERROR;
  }

  dest_addr.sin_family = AF_INET;  

  if ((inaddr = inet_addr(ip)) == INADDR_NONE) {  
    if ((pHost = gethostbyname(ip)) == NULL) {  
      perror("gethostbyname()");  
      goto PING_ERROR;
    }

    memmove(&dest_addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);  
  }  
  else {  
    memmove(&dest_addr.sin_addr, &inaddr, sizeof(struct in_addr));
  }  
  
  int unpack_ret;  
        
  if(SendPacket(sock_icmp, &dest_addr, 1) < 0)
    goto PING_ERROR;
          
    
  if(RecvePacket(sock_icmp, &dest_addr) < 0)
    goto PING_ERROR;

  close(sock_icmp);
  return true;

PING_ERROR:
  close(sock_icmp);
  return false;
}

}
