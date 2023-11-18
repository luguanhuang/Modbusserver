#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
//#include <my_global.h>
#include <mysql.h>

static const uint8_t table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const uint8_t table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};
int getuser(char buff[]);
static uint16_t crc16(uint8_t *crbuffer, uint16_t crbuffer_length);
//static float modbus_get_float_abcd(const uint16_t *src);
static float modbus_get_float(const uint16_t u16A,const uint16_t u16B);
static float modbus_get_float_abcd(const uint16_t u16A,const uint16_t u16B);
static float modbus_get_float_dcba(const uint16_t u16A,const uint16_t u16B);
static float modbus_get_float_badc(const uint16_t u16A,const uint16_t u16B);
static float modbus_get_float_cdab(const uint16_t u16A,const uint16_t u16B);
int store_result_coil(char user[],int ch,int nb,int dtype,int res_length,char bbuuff[],int proto);
int store_result_reg(char user[],int ch,int nb,int datatype,int res_length,char bbuuff[],int proto);
uint16_t bswap_16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}
uint32_t bswap_32(uint32_t x)
{
    return (bswap_16(x & 0xffff) << 16) | (bswap_16(x >> 16));
}

#define PORT 13000
//#define retry 3
int newsocket;
int sockfd,ret;
struct sockaddr_in serverAddr;
struct sockaddr_in newAddr;
char buffer[256];
uint8_t bbuuff[256];
pid_t childpid;
socklen_t size_addr;
fd_set rset; 
struct timeval timeout;
uint8_t reqq[256];
//uint8_t *reqq;
char *requ;
char user[50];
int protocol; int retrycount=0; int retrynb; int polltime; int tmout;

int main(){
    sockfd =socket(AF_INET,SOCK_STREAM,0);

    if(sockfd < 0){
    printf("[+]error in connection");
    exit(1);
                   }
    printf("[+]Server socket is created \n");
    memset(&serverAddr,'\0',sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    //serverAddr.sin_addr.s_addr = inet_addr("192.168.43.209");
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
    printf("[+]Error in Binding socket \n");
    exit(1);
    }
printf("[+]Binding socket \n");
if (listen(sockfd,100) < 0) {
        printf("[+]listen error:%d\n",10000);
    }
printf("[+]Listning on port %d \n",PORT);
while(1){

    newsocket = accept(sockfd,(struct sockaddr *)&newAddr,&size_addr);
    if(newsocket <0)
            {
        close(sockfd);        
        printf("[+]socket accept error\n");
        exit(1);
            }
    else{printf("[+]connection accepted from %s : %d \n",inet_ntoa(newAddr.sin_addr),ntohs(newAddr.sin_port));}

    
    if((childpid = fork()) == 0)
           {
        close(sockfd);
        bzero(buffer,sizeof(buffer));
        recv(newsocket,buffer,1024,0);
        //printf("value of buffer is %s",buffer);
        //send(newsocket,buffer,strlen(buffer),0);
        int keepcnt = 5;
        int keepidle = 30;
        int keepintvl = 120;
       
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int));
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int));
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(int));
       while(1)
        {
            
            int i;int getuserval; 
            printf("received buffer is %s\n",buffer);
            uint16_t tflag=0;

            getuserval = getuser(buffer);

            if (getuserval < 0)
            {
                shutdown(newsocket,SHUT_RDWR);
                close(newsocket);

            }

           
                    struct timeval timeout;
                    //timeout.tv_sec = 5;
                    timeout.tv_sec = (tmout/1000);
                    timeout.tv_usec = 0;

                if(tflag >255){tflag = 0;}    

                MYSQL *con = mysql_init(NULL);
                if (con == NULL) 
                  {
                 fprintf(stderr, "%s\n", mysql_error(con));
                 exit(1);
                 }
                if (mysql_real_connect(con, "localhost", "root", "root1", 
                "cloud", 0, NULL, 0) == NULL) 
                {
                fprintf(stderr, "%s\n", mysql_error(con));
                mysql_close(con);
                exit(1);
                }

         /*--------------- program start after protocol select-------*/
         
            if(protocol == 1)
              {

                 if(retrycount >= retrynb)
            {
                shutdown(newsocket,SHUT_RDWR);close(newsocket); exit(1);
            }

                 //modbus write part if pending
                   char wq[256];
                   uint8_t req_wr[256];
                   sprintf(wq,"SELECT id,channel,dtype,slaveid,funcode,reg,value from %s_write WHERE mbwrite=('0')",user); 
                   if (mysql_query(con, wq)) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      printf("error 2 %d",newsocket);
                      mysql_close(con);
                      shutdown(newsocket,SHUT_RDWR);
                      close(newsocket);
                      exit(1);
                  }
                  MYSQL_RES *resultw = mysql_store_result(con);
                  if (resultw == NULL) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 3");
                  }

                  MYSQL_ROW roww;
            while((roww = mysql_fetch_row(resultw))) 
            { 
                    int id = atoi(roww[0]);
                    int dtype = atoi(roww[2]);
                    int sl_id = atoi(roww[3]);
                    int funcode = atoi(roww[4]);
                    int reg = (atoi(roww[5])-1);
                    int val; int req_lenwoCRC;
                    int size_float=4;
                    req_wr[0] = sl_id;
                    req_wr[1] = funcode;
                    req_wr[2] = reg >> 8;
                    req_wr[3] = reg & 0x00ff;
                    
                    if(dtype == 1 && funcode == 5)
                    {                      
                        req_lenwoCRC =6;
                      if(atoi(roww[6]) > 0)
                           {
                            req_wr[4] = 0XFF;
                            req_wr[5] = 0x00;
                           }
                      else{
                            req_wr[4] = 0X00;
                            req_wr[5] = 0x00;
                          }

                    }
                    //float write values
                    if((dtype == 4 | dtype == 5 | dtype == 6 | dtype == 7 | dtype == 8) && funcode == 16)
                    {   
                        //uint16_t req_w_16;
                        req_wr[4] = 0X00;
                        req_wr[5] = 0X02;
                        req_wr[6] = size_float;                 
                        req_lenwoCRC = 6 + 1 + size_float;//1 is extra byte to get data byte receive
                        uint32_t ik;
                        uint16_t dest[2];
                        float fir = atof(roww[6]);
                        memcpy(&ik,&fir, sizeof(uint32_t));
                       if(dtype == 4) 
                         {
                             ik = htonl(ik); 
                             dest[0] = ik;
                             dest[1] = ik >> 16;
                          }
                         if(dtype == 5) 
                          {
                            ik = htonl(ik);
                             dest[0] = ik >> 16;
                             dest[1] = ik ;
                          }
                        if(dtype == 6) 
                        {
                            ik = bswap_32(htonl(ik));
                             dest[0] = ik >> 16;
                             dest[1] = ik ;
                        }
                       if(dtype == 7) 
                        {
                            ik = htonl(ik);
                            dest[0] = (uint16_t)bswap_16(ik >> 16);
                            dest[1] = (uint16_t)bswap_16(ik & 0xFFFF);
                        }
                        if(dtype == 8) 
                       {
                             //ik = htonl(ik);
                             dest[0] = ik;
                             dest[1] = ik >> 16;
                           
                        }        
                       
                        printf("dest[0] %.4X",dest[0]);
                        printf("dest[1] %.4X",dest[1]);
                        req_wr[7] = dest[0] ;
                        req_wr[8] = dest[0]>> 8;
                        req_wr[9] = dest[1] ;
                        req_wr[10] = dest[1]>> 8;

                    }
                    int write_success =0;
                    uint16_t crcw = crc16(req_wr,req_lenwoCRC);
                    req_wr[req_lenwoCRC++] = crcw >> 8;
                    req_wr[req_lenwoCRC++] = crcw & 0X00FF;
                     FD_ZERO(&rset);
                     FD_SET(newsocket,&rset); 
                     int rb=0;int rsw;
                     //timeout.tv_sec = 5;
                     //timeout.tv_usec = 0;
                     timeout.tv_sec = (tmout/1000);
                     timeout.tv_usec = 0;
                     send(newsocket,req_wr,req_lenwoCRC,0);
                     
                     rsw=select(newsocket+1,&rset,NULL,NULL,&timeout);
                     
                     if(FD_ISSET(newsocket,&rset))
                     {
                       bzero(bbuuff,sizeof(bbuuff));
                       rb=recv(newsocket,bbuuff,req_lenwoCRC,0);
                     }  
                     if(rb<0) {printf("Socket Error");shutdown(newsocket,SHUT_RDWR);close(newsocket); exit(1);}
                     if(rb != 0)
                     {   
                     for(i=0;i<6;i++)
                     {  
                        if(bbuuff[i] == req_wr[i])
                        {
                            write_success =1;
                        }
                        else{write_success =0;}

                     }
                     }
                     //if(rb == req_lenwoCRC )
                     if(write_success == 1)
                     {
                        char wqu[256];
                        sprintf(wqu,"UPDATE %s_write set mbwrite=('1') where id=%d",user,id); 
                         
                        if (mysql_query(con, wqu)) 
                            {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 2");
                     // mysql_close(con);
                      //exit(1);
                            }
                            else(printf("database update success"));
                     }
                  } 

                





                char q[1024];
                sprintf(q,"SELECT ch,dtype,slaveid,funcode,startreg,countreg from %s WHERE active=('1')",user); 
                if (mysql_query(con, q)) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 2");
                     // mysql_close(con);
                      //exit(1);
                  }
                  MYSQL_RES *result1 = mysql_store_result(con);
                  if (result1 == NULL) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 3");
                  }
                  int num_fields1 = mysql_num_fields(result1);                
                  MYSQL_ROW row1;
                  MYSQL_FIELD *field1;
                  char que[50]; 
                  int slave_id,functioncode,addr,nb;                 
                  while((row1 = mysql_fetch_row(result1))) 
                  { 
                    int ch = atoi(row1[0]);
                    int datatype = atoi(row1[1]);
                    slave_id = atoi(row1[2]);
                    functioncode = atoi(row1[3]);
                    addr = atoi(row1[4])-1;
                    nb = atoi(row1[5]);
                    reqq[0] = slave_id;
                    reqq[1] = functioncode;
                    reqq[2] =   addr >> 8;
                    reqq[3] = addr & 0x00ff;
                    reqq[4] =   nb  >> 8;
                    reqq[5] =  nb & 0x00ff;
                    int rq_header_length = 2;
                    int addr_length =2;
                    int nb_length = 2;
                    int req_length =rq_header_length+addr_length+nb_length;
                    int crc_length =2;  
                    uint16_t crc = crc16(reqq, req_length);

                    reqq[req_length++] = crc >> 8;
                    reqq[req_length++] = crc & 0x00FF;

                    int res_length;
                    fd_set rset;
                   // struct timeval timeout;
                  //  timeout.tv_sec = 5;
                   // timeout.tv_usec = 0;
                    timeout.tv_sec = (tmout/1000);
                    timeout.tv_usec = 0;
                    int rb=0;
                send(newsocket,reqq,req_length,0);//function to send query
                    int  rsp_header_length= 5;
                        /*for float read response length*/
                        if(functioncode == 3 | functioncode == 4)
                        {
                             
                     res_length = (nb*2)+rsp_header_length;
                            
                        }
                    /*for coil read response length*/
                      if(functioncode == 1 | functioncode == 2)
                        {
                    res_length = rsp_header_length + (nb / 8) + ((nb % 8) ? 1 : 0);
                         
                        }
                     FD_ZERO(&rset);
                     FD_SET(newsocket, &rset); 
                     int rr;
                     rr=select(newsocket+1,&rset,NULL,NULL,&timeout);
                     //printf("select value %d \n",rr);
                    printf("request is %s \n",reqq);
                    printf("response length should be %d \n",res_length);
                            
                  if (FD_ISSET(newsocket,&rset))
                     {
                  
                                      
                         bzero(bbuuff,sizeof(bbuuff));
                         rb=recv(newsocket,bbuuff,res_length,0);

                        printf("length of received item is%d \n",rb );
                         //printf("response is %s",bbuuff);
                    
                      }//close loop of FD_SET recv
                      if(rb<0) {printf("Socket Error");shutdown(newsocket,SHUT_RDWR);close(newsocket); exit(1);}
                      if(rb == 0){
                                printf("retry is %d \n",retrycount); 
                                retrycount++;
                                }
                            else{
                                retrycount = 0;
                                }

                      if(rb == res_length)
                        {
                    
                        //printf("received success %s \n",bbuuff);
                        printf("received success\n");
                        for(i=0;i<rb;i++)
                        {
                        printf("<%.2X>", (unsigned char)bbuuff[i]);
                        }
                 if(bbuuff[0] == slave_id && bbuuff[1]==functioncode)
                    {   
                    uint16_t crc_calculated = crc16(bbuuff,res_length - 2);
                    uint16_t crc_received = (bbuuff[res_length - 2] << 8) | bbuuff[res_length - 1];
                    if(crc_calculated == crc_received)
                        {
                            if(bbuuff[1] == 3 | bbuuff[1] == 4) 
                             { 

                            store_result_reg(user,ch,nb,datatype,res_length,bbuuff,protocol);
                             }   
                            if(bbuuff[1] == 1 | bbuuff[1] == 2)
                             {   
                            store_result_coil(user,ch,nb,datatype,res_length,bbuuff,protocol);
                             }   

                        }
                      else{printf("CRC received error\n");}  
                    }        
                        }//rb==length loop close                      
            }//while loop data send close
                  bzero(buffer,sizeof(buffer));
                  mysql_free_result(result1);
                  mysql_close(con);
         } //protocol select close loop 

     if(protocol == 2)
              {

                 if(retrycount >= retrynb)
            {
                shutdown(newsocket,SHUT_RDWR);close(newsocket); exit(1);
            }

                 //modbus write part if pending
                   char wq[256]; 
                   uint8_t req_wr[256];
                   
                   sprintf(wq,"SELECT id,channel,dtype,slaveid,funcode,reg,value from %s_write WHERE mbwrite=('0')",user); 
                   if (mysql_query(con, wq)) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      printf("error 2 %d",newsocket);
                      mysql_close(con);
                      shutdown(newsocket,SHUT_RDWR);
                      close(newsocket);
                      exit(1);
                  }
                  MYSQL_RES *resultw = mysql_store_result(con);
                  if (resultw == NULL) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 3");
                  }

                  MYSQL_ROW roww;
                int  res_length_offset;
            while((roww = mysql_fetch_row(resultw))) 
            { 
                    uint8_t mbtcpoffset = 6;
                    int id = atoi(roww[0]);
                    int dtype = atoi(roww[2]);
                    int sl_id = atoi(roww[3]);
                    int funcode = atoi(roww[4]);
                    int reg = (atoi(roww[5])-1);
                    int val; int req_lenwoCRC;
                    int size_float=4;
                    req_wr[0] = tflag >> 8;
                    req_wr[1] = tflag;
                    req_wr[2] =0X00;
                    req_wr[3] =0X00;
                    req_wr[mbtcpoffset++] = sl_id;
                    req_wr[mbtcpoffset++] = funcode;
                    req_wr[mbtcpoffset++] = reg >> 8;
                    req_wr[mbtcpoffset++] = reg & 0x00ff;
                    
                    if(dtype == 1 && funcode == 5)
                    {                      
                        req_lenwoCRC =6+1+4;
                        res_length_offset =6;
                      if(atoi(roww[6]) > 0)
                           {
                            req_wr[mbtcpoffset++] = 0XFF;
                            req_wr[mbtcpoffset++] = 0x00;
                           }
                      else{
                            req_wr[mbtcpoffset++] = 0X00;
                            req_wr[mbtcpoffset++] = 0x00;
                          }

                    }
                    //float write values
                    if((dtype == 4 | dtype == 5 | dtype == 6 | dtype == 7 | dtype == 8) && funcode == 16)
                    {   
                        //uint16_t req_w_16;
                        res_length_offset =6;
                        req_wr[mbtcpoffset++] = 0X00;
                        req_wr[mbtcpoffset++] = 0X02;
                        req_wr[mbtcpoffset++] = size_float;                 
                        req_lenwoCRC = 6+1 + 1 + size_float;//1 is extra byte to get data byte receive
                        uint32_t ik;
                        uint16_t dest[2];
                        float fir = atof(roww[6]);
                        memcpy(&ik,&fir, sizeof(uint32_t));
                       if(dtype == 4) 
                         {
                             ik = htonl(ik); 
                             dest[0] = ik;
                             dest[1] = ik >> 16;
                          }
                         if(dtype == 5) 
                          {
                            ik = htonl(ik);
                             dest[0] = ik >> 16;
                             dest[1] = ik ;
                          }
                        if(dtype == 6) 
                        {
                            ik = bswap_32(htonl(ik));
                             dest[0] = ik >> 16;
                             dest[1] = ik ;
                        }
                       if(dtype == 7) 
                        {
                            ik = htonl(ik);
                            dest[0] = (uint16_t)bswap_16(ik >> 16);
                            dest[1] = (uint16_t)bswap_16(ik & 0xFFFF);
                        }
                        if(dtype == 8) 
                       {
                             //ik = htonl(ik);
                             dest[0] = ik;
                             dest[1] = ik >> 16;
                           
                        }        
                       
                        printf("dest[0] %.4X",dest[0]);
                        printf("dest[1] %.4X",dest[1]);
                        req_wr[mbtcpoffset++] = dest[0] ;
                        req_wr[mbtcpoffset++] = dest[0]>> 8;
                        req_wr[mbtcpoffset++] = dest[1] ;
                        req_wr[mbtcpoffset++] = dest[1]>> 8;

                    }
                    req_wr[4] = (mbtcpoffset - 6) >> 8 ;
                    req_wr[5] = (mbtcpoffset - 6)  ;
                    int write_success =0;
                    //uint16_t crcw = crc16(req_wr,req_lenwoCRC);
                    //req_wr[req_lenwoCRC++] = crcw >> 8;
                    //req_wr[req_lenwoCRC++] = crcw & 0X00FF;
                     FD_ZERO(&rset);
                     FD_SET(newsocket,&rset); 
                     int rb=0;int rsw;
                     //timeout.tv_sec = 5;
                     //timeout.tv_usec = 0;
                     timeout.tv_sec = (tmout/1000);
                     timeout.tv_usec = 0;
                     send(newsocket,req_wr,mbtcpoffset,0);
                     
                     rsw=select(newsocket+1,&rset,NULL,NULL,&timeout);
                     
                     if(FD_ISSET(newsocket,&rset))
                     {
                       bzero(bbuuff,sizeof(bbuuff));
                       rb=recv(newsocket,bbuuff,req_lenwoCRC,0);
                     }
                     if(rb<0) {printf("Socket Error");shutdown(newsocket,SHUT_RDWR);close(newsocket); exit(1);}
                     if(rb != 0)
                     {   
                     for(i=6;i<12;i++)
                     {  
                        if(bbuuff[i] == req_wr[i])
                        {
                            write_success =1;
                            printf("bbuuff<%d><%.2X>",i, (unsigned char)bbuuff[i]);
                            printf("req_wr<%d><%.2X>",i, (unsigned char)req_wr[i]);
                        }

                        else{write_success =0;}
                    }
                     }
                     //if(rb == req_lenwoCRC )
                     if(write_success == 1)
                     {
                        char wqu[256];
                        sprintf(wqu,"UPDATE %s_write set mbwrite=('1') where id=%d",user,id); 
                         
                        if (mysql_query(con, wqu)) 
                            {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 2");
                     // mysql_close(con);
                      //exit(1);
                            }
                            else(printf("database update success"));
                     }
                     tflag++;
                  } 

                


                  

               
                char q[1024];
                sprintf(q,"SELECT ch,dtype,slaveid,funcode,startreg,countreg from %s WHERE active=('1')",user); 
                if (mysql_query(con, q)) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 2");
                     // mysql_close(con);
                      //exit(1);
                  }
                  MYSQL_RES *result1 = mysql_store_result(con);
                  if (result1 == NULL) 
                  {
                      fprintf(stderr, "%s\n", mysql_error(con));
                      //printf("error 3");
                  }
                  int num_fields1 = mysql_num_fields(result1);                
                  MYSQL_ROW row1;
                  MYSQL_FIELD *field1;
                  char que[50]; 
                  int slave_id,functioncode,addr,nb;                 
                  while((row1 = mysql_fetch_row(result1))) 
                  { 
                    uint8_t mbtcpoffset = 6;
                    int ch = atoi(row1[0]);
                    int datatype = atoi(row1[1]);
                    slave_id = atoi(row1[2]);
                    functioncode = atoi(row1[3]);
                    addr = atoi(row1[4])-1;
                    nb = atoi(row1[5]);
                    reqq[0] = tflag >> 8;
                    reqq[1] = tflag;
                    reqq[2] =0X00;
                    reqq[3] =0X00;
                    reqq[mbtcpoffset++] = slave_id;
                    reqq[mbtcpoffset++] = functioncode;
                    reqq[mbtcpoffset++] =   addr >> 8;
                    reqq[mbtcpoffset++] = addr & 0x00ff;
                    reqq[mbtcpoffset++] =   nb  >> 8;
                    reqq[mbtcpoffset++] =  nb & 0x00ff;
                    int rq_header_length = 2;
                    int addr_length =2;
                    int nb_length = 2;
                    int req_length =(rq_header_length+addr_length+nb_length)+6;
                    int crc_length =2;  
                    //uint16_t crc = crc16(reqq, req_length);

                   // reqq[req_length++] = crc >> 8;
                   // reqq[req_length++] = crc & 0x00FF;
                    reqq[4] = (mbtcpoffset -6 ) >> 8;
                    reqq[5] = (mbtcpoffset -6 );
                    int res_length;
                    fd_set rset;
                   // struct timeval timeout;
                  //  timeout.tv_sec = 5;
                   // timeout.tv_usec = 0;
                    timeout.tv_sec = (tmout/1000);
                    timeout.tv_usec = 0;
                    int rb=0;
        send(newsocket,reqq,req_length,0);//function to send query
                    int  rsp_header_length= 5;
                        /*for float read response length*/
                       if(functioncode == 3 | functioncode == 4)
                        {
                             
                     res_length = ((nb*2)+rsp_header_length)+6;
                            
                        }
                    /*for coil read response length*/
                     if(functioncode == 1 | functioncode == 2)
                        {
                    res_length = (rsp_header_length + (nb / 8) + ((nb % 8) ? 1 : 0))+6;
                         
                        }
                     FD_ZERO(&rset);
                     FD_SET(newsocket, &rset); 
                     int rr;
                     rr=select(newsocket+1,&rset,NULL,NULL,&timeout);
                     //printf("select value %d \n",rr);
                    printf("request is %s \n",reqq);
                    printf("response length should be %d \n",(res_length-2));
                            
                  if (FD_ISSET(newsocket,&rset))
                     {
                  
                                      
                         bzero(bbuuff,sizeof(bbuuff));
                         rb=recv(newsocket,bbuuff,res_length,0);

                        printf("length of received item is%d \n",rb );
                         //printf("response is %s",bbuuff);
                    
                      }//close loop of FD_SET recv
                      if(rb<0) {printf("Socket Error");shutdown(newsocket,SHUT_RDWR);close(newsocket); exit(1);}
                      if(rb == 0){
                                printf("retry is %d \n",retrycount); 
                                retrycount++;
                                }
                            else{
                                retrycount = 0;
                                }

                      if(rb == (res_length-2))
                        {
                    
                        //printf("received success %s \n",bbuuff);
                        printf("received success\n");
                        for(i=0;i<rb;i++)
                        {
                        printf("<%.2X>", (unsigned char)bbuuff[i]);
                        }
                 if(bbuuff[6] == slave_id && bbuuff[7]==functioncode)
                    {   
                    //uint16_t crc_calculated = crc16(bbuuff,res_length - 2);
                    //uint16_t crc_received = (bbuuff[res_length - 2] << 8) | bbuuff[res_length - 1];
                    //if(crc_calculated == crc_received)
                      //  {
                            if(bbuuff[7] == 3 | bbuuff[7] == 4) 
                             { 

                            store_result_reg(user,ch,nb,datatype,res_length,bbuuff,protocol);
                             }   
                            if(bbuuff[7] == 1 | bbuuff[7] == 2)
                             {   
                            store_result_coil(user,ch,nb,datatype,res_length,bbuuff,protocol);
                             }   

                        //}
                     // else{printf("CRC received error\n");}  
                    }        
                        }//rb==length loop close                  
            }//while loop data send close
                  bzero(buffer,sizeof(buffer));
                  mysql_free_result(result1);
                  mysql_close(con);
         } //protocol select close loop     


        }// while loop close after child process fork   

   }//childpid if close
}// while accept loop close
close(newsocket);
return 0;
}// main loop close

int store_result_reg(char user[],int ch,int nb,int datatype,int res_length,char bbuuff[],int proto)

{
    uint16_t tab_registers[128];
    float f[128];
    int floatoffset =0;
    int i=0;
   // printf("\n value of Nb is %d \n\n\n",nb);  
    
    int offset;

    
    for (i=0;i<nb;i++)
    {
     
     if(proto == 1){offset = (i*2);}
     if(proto == 2){offset = ((i*2)+6);}

    tab_registers[i] = (unsigned char)bbuuff[offset+3] << 8 | (unsigned char)bbuuff[offset+4];
 //   printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+3]);
 //   printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+4]);
  //  printf("\n tab register<%d> value is %.4X \n",i,tab_registers[i]);
    i=i+1;
    //offset =i*2; 
    if(proto == 1){offset = (i*2);}
     if(proto == 2){offset = ((i*2)+6);}   
  //  printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+3]);
  //  printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+4]);    
    tab_registers[i] = (unsigned char)bbuuff[offset+3] << 8 | (unsigned char)bbuuff[offset+4];
 //   printf("\n tab register<%d> value is %.4X \n",i,tab_registers[i]);
    if(datatype == 4) 
           {
        f[floatoffset]= modbus_get_float(tab_registers[i-1],tab_registers[i]);
            }
    if(datatype == 5) 
           {
        f[floatoffset]= modbus_get_float_badc(tab_registers[i-1],tab_registers[i]);    
    }
        
    if(datatype == 6) 
           {
        f[floatoffset]= modbus_get_float_abcd(tab_registers[i-1],tab_registers[i]);
            }
        
            
    if(datatype == 7) 
           {
        f[floatoffset]= modbus_get_float_dcba(tab_registers[i-1],tab_registers[i]);
            }
    if(datatype == 8) 
           {
        f[floatoffset]= modbus_get_float_cdab(tab_registers[i-1],tab_registers[i]);
            }        
 //   printf("\n tab register<%d> value is %.4X%.4X \n",i,tab_registers[i-1],tab_registers[i]);
  //  printf("f<%d>float value is %f \n",floatoffset,f[floatoffset]);
    floatoffset++;
                
    }
        MYSQL *con = mysql_init(NULL);
        if (con == NULL) 
        {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
        }
        if (mysql_real_connect(con, "localhost", "root", "root1", 
                          "cloud", 0, NULL, 0) == NULL) 
         {
         fprintf(stderr, "%s\n", mysql_error(con));
         mysql_close(con);
         exit(1);
         } 

       int k=0;
       int lastid;
       char query2[300];
       sprintf(query2," INSERT INTO %s_%d (`1`) values ('%f')",user,ch,f[0]);
     //  printf("%s",query2);
       if (mysql_query(con, query2)) 
            {
            fprintf(stderr, "%s\n", mysql_error(con));
            printf("\nerror\n");
            //mysql_close(con);
            //exit(1);
            }
        else{
            printf("success insert values in database");
            lastid = mysql_insert_id(con);
            printf("last insert id is %d",lastid);
            }
            for(k=0;k<(nb/2);k++)
                {
                char query3[300];
                sprintf(query3," UPDATE %s_%d set `%d` = %f where id=%d",user,ch,(k*2)+1,f[k],lastid);
                
                if (mysql_query(con, query3)) 
                    {
                    fprintf(stderr, "%s\n", mysql_error(con));
                    //mysql_close(con);
                    //exit(1);
                    }
                    
                }
                
                mysql_close(con);
                       
}//function close



int store_result_coil(char user[],int ch,int nb,int dtype,int res_length,char bbuuff[],int proto)

{

        int i;
        uint8_t rsp_io_byte[32];
        int calc_length = res_length -5;
        int k=0;
        int j=0;
        char bits[256];
        int offset;
        if(proto == 1){offset = 0;}
        if(proto == 2){offset = 6;}

        for(i=0;i<calc_length;i++)
         {                      
         int mask =1;
         rsp_io_byte[i] = bbuuff[i+offset+3];
        // printf("bbuuff<%.2X> \n",rsp_io_byte[i] );
         for(k=j;k<j+8;k++)
           {

             bits[k] = rsp_io_byte[i] >> (k-(i*8)) & mask;
           //  printf("bits[%d]%d \n",k,bits[k]);
           } //for loop of K close
           j=k;
          }// for loop of i close
     MYSQL *con = mysql_init(NULL);
    if (con == NULL) 
        {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
        }
        if (mysql_real_connect(con, "localhost", "root", "root1", 
                          "cloud", 0, NULL, 0) == NULL) 
         {
         fprintf(stderr, "%s\n", mysql_error(con));
         mysql_close(con);
         exit(1);
         }     
     int lastid;
     char query2[300];
     int n;
    sprintf(query2," INSERT INTO %s_%d (`1`) values ('%d')",user,ch,bits[0]);
         if (mysql_query(con, query2)) 
            {
            fprintf(stderr, "%s\n", mysql_error(con));
           //mysql_close(con);
           //exit(1);
            }
            else{
                printf("success insert values in database");
                lastid = mysql_insert_id(con);
                printf("last insert id is %d",lastid);
                }
        for(n=0;n<nb;n++)
        {

        char query3[300];
        sprintf(query3," UPDATE %s_%d set `%d` = ('%d') where id=%d",user,ch,n+1,bits[n],lastid);
        if (mysql_query(con, query3)) 
           {
           fprintf(stderr, "%s\n", mysql_error(con));
           //mysql_close(con);
           //exit(1);
           
           } //mysql query update close
         else{
           //     printf ("database update %d<%d>",n+1,bits[n]);  
             }  
           
        } //n close
         mysql_close(con);              
}//function close

int getuser(char buff[])
{
    
   // printf("value of buff is %s",buff);
    MYSQL *con = mysql_init(NULL);
    if (con == NULL) 
        {
        fprintf(stderr, "%s\n", mysql_error(con));
        return -1;
       // exit(1);
        }
        if (mysql_real_connect(con, "localhost", "root", "root1", 
                          "cloud", 0, NULL, 0) == NULL) 
         {
         fprintf(stderr, "%s\n", mysql_error(con));
         mysql_close(con);
         return -1;
         //exit(1);
         }
         //printf("stage1 ");
         char query[300];
         sprintf(query,"SELECT user,ptype,retry,timeout from auth where macid=('%s')",buff); 
         if (mysql_query(con, query)) 
            {
            fprintf(stderr, "%s\n", mysql_error(con));
            printf("error 2");
            mysql_close(con);

            return -1;
            }
          MYSQL_RES *result = mysql_store_result(con);
          if (result == NULL) 
            {
            fprintf(stderr, "%s\n", mysql_error(con));
            return -1;
           //printf("error 3");
            }
           int num_fields = mysql_num_fields(result);
           MYSQL_ROW row;
           MYSQL_FIELD *field;
           while((row = mysql_fetch_row(result))) 
                  {                
                    //send(newsocket,row[0],strlen(row[0]),0);
                    strcpy(user,row[0]);
                    protocol = atoi(row[1]);
                    retrynb = atoi(row[2]);
                    tmout = atoi(row[3]);
                    printf(" user is %s",user);
                    printf(" protocol is %d",protocol);
                    printf("retrycount is %d",retrynb);
                    printf(" timeout is %d",tmout);
                  } 
                  mysql_free_result(result);
                  mysql_close(con);
                  return 0;
}     //close function


static uint16_t crc16(uint8_t *crbuffer, uint16_t crbuffer_length)
{
    uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (crbuffer_length--) {
        i = crc_hi ^ *crbuffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}


float modbus_get_float(const uint16_t u16A,const uint16_t u16B)
{
    float f;
    uint32_t i;

   // i = ntohl(((uint32_t)src[0] << 16) + src[1]);
    i = (((uint32_t)u16A << 16)) + u16B   ;

    printf("i value is %.8X",i);

    memcpy(&f, &i, sizeof(float));
    //float *f = (float *)&i;
    
    //float f = 100.123;
    return f;

}  
float modbus_get_float_abcd(const uint16_t u16A,const uint16_t u16B)
{
    float f;
    uint32_t i;
    i = ntohl(((uint32_t)u16A << 16)) + u16B ;
    memcpy(&f, &i, sizeof(float));
    return f;

}   
float modbus_get_float_dcba(const uint16_t u16A,const uint16_t u16B)
{
    float f;
    uint32_t i;
    i = ntohl(bswap_32((((uint32_t)u16A) << 16) + u16B)) ;
    memcpy(&f, &i, sizeof(float));
    return f;

}
float modbus_get_float_badc(const uint16_t u16A,const uint16_t u16B)
{
    float f;
    uint32_t i;

    i = ntohl((uint32_t)(bswap_16(u16A) << 16) + bswap_16(u16B));
    memcpy(&f, &i, sizeof(float));

    return f;
}
float modbus_get_float_cdab(const uint16_t u16A,const uint16_t u16B)
{
    float f;
    uint32_t i;

    i = ntohl((((uint32_t)u16A) << 16) + u16B);
    memcpy(&f, &i, sizeof(float));

    return f;
}                          
