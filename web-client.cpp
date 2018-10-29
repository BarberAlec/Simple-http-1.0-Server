//
//Developed by Alec Barber. Aayush Madaan
//Jan/Feb 2018
//
//  SYNTAX
//  ./web-client [URL] [URL] [URL] ...
//  example : ./web-client http://localhost:4000/curr_folder/index.txt 
//  client will save file localy and name it recievedFile.txt, client will overwrite any file with
//  the same name.
//
#include <string>
#include <string.h>
#include <thread>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#define STR_LEN 200
#define LNG_STR_LEN 1048576
#define LISTEN_BACKLOG 10

void URLtoIP(char* url);

class HTTPrequest{
  public:
    struct sockaddr_in servAddr;          
    int SockHandle;                       //Handle for open socket
    int port;                             //port number of current request
    char targetIP[STR_LEN];               //IP of server you are tryingg to connect to
    char targetURL[STR_LEN];              //URL of target server

    HTTPrequest();
    int openConnection(int set1, int set2, int port);               //sets up some initialisers
    int requestFile(char Fname[STR_LEN], char destFname[STR_LEN]);  //main body of code that sends request for a file and parses response
    void closeConnection();                                         //closes connection, when finished
  private:
    void setSocketAndAddr(int set1, int set2, int port);            //sub methods that are used to set up variable etc.
    void setIP(int set1, int set2);
    int readHeader(char* temp);
};

inline HTTPrequest::HTTPrequest(){
  memset(targetIP, '\0', sizeof(targetIP));
}

inline int HTTPrequest::openConnection(int set1, int set2, int port){
  setSocketAndAddr(set1, set2, port);
  setIP(set1, set2);
  if(inet_aton(targetIP, &servAddr.sin_addr) == -1){
    perror("Client: Error: IP Address failed to attach to socket.\n");
    return -1;
  }
  if(connect(SockHandle,(struct sockaddr*)&servAddr,sizeof(servAddr)) == -1){
    perror("Client: Error: could not connect to server\n");
    return -1;
  }
  
  std::cout<<"Client: Connected to server"<<std::endl;
  return 0;
}

inline int HTTPrequest::requestFile(char Fname[STR_LEN], char destFname[STR_LEN]){
  char temp[LNG_STR_LEN];
  strcpy(temp,"GET ");
  strcat(temp, Fname);
  strcat(temp, " HTTP/1.0\r\n\r\n");
  std::cout<<"Client: Requesting :"<<temp<<std::endl;
  if(send(SockHandle, temp, sizeof(temp), 0) == -1){
    perror("Client: Error: Request message not sent\n");
    return -1;
  }

  memset(temp,'\0',sizeof(temp));
  if(recv(SockHandle,temp,sizeof(temp),0) == -1){
    perror("Client: Error: Data not recieved using recv...\n");
    return -1;
  }
  if(readHeader(temp) == 200){                    //reads head and outputs state to console
    std::cout<<"Saving file..."<<std::endl<<std::endl;
    FILE * info = fopen(destFname,"w+");
    fprintf(info,"%s",temp);
    fclose(info);
  }
  
  return 0;
}

inline void HTTPrequest::setSocketAndAddr(int set1, int set2, int port){
  SockHandle = socket(set1, set2, 0);
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
}

inline void HTTPrequest::setIP(int set1, int set2){
  struct addrinfo info;
  struct addrinfo* Pinfo;
  memset(&info, 0, sizeof(info));
  info.ai_family = set1;
  info.ai_socktype = set2;
  if(getaddrinfo(targetURL, "80", &info, &Pinfo) != 0){
    perror("Client: Error: URLToIP Failed\n");
    exit(1);
  }
  struct sockaddr_in* IPv4 = (struct sockaddr_in*)Pinfo->ai_addr;
  inet_ntop(info.ai_family,&(IPv4->sin_addr),targetIP,sizeof(targetIP));
}

inline void HTTPrequest::closeConnection(){
  close(SockHandle);
}

inline int HTTPrequest::readHeader(char* temp){
  int headNum;
  char cNum[STR_LEN];
  memset(cNum, '\0', sizeof(cNum));
  cNum[0] = temp[9];
  cNum[1] = temp[10];
  cNum[2] = temp[11];
  headNum = std::atoi(cNum);
  switch(headNum){
    case 200 :
      std::cout<<"Client: Recieved file, all ok"<<std::endl;
    break;
    case 400 :
      std::cout<<"Client: Recieved Bad Request response"<<std::endl;
    break;
    case 404 :
      std::cout<<"Client: Recieved A Could Not Find File Response"<<std::endl;
    break;
    default :
      std::cout<<"Client: Error: Did not recognize header number response"<<std::endl;
    break;
  }
  int i = 0;
  int count = 0;
  while(temp[i] != '\n' || count != 1){
    if(temp[i] == '\n')count++;
    i++;

  }
  char* tmp = &temp[i+1];
  strcpy(temp, tmp);
  std::cout<<"Client: Header string: "<<headNum<<std::endl;
  return headNum;
}

int main(int argc, char *argv[])
{
  int prt;
  char temp1[STR_LEN];
  char temp2[STR_LEN];
  char holder[STR_LEN];
  char url[STR_LEN];
  char fName[STR_LEN];
  memset(temp1, '\0',sizeof(temp1));
  memset(fName, '\0', sizeof(fName));
  memset(url, '\0',sizeof(url));
  if(argc == 1){
    strcpy(url, "localhost");
    strcpy(holder, "/");
    prt = 4000;
    HTTPrequest myRequest;
    strcpy(myRequest.targetURL,url);
    if(myRequest.openConnection(AF_INET, SOCK_STREAM, prt) == -1){
      perror("Client: Error: openConnection failure!!!");
      return -1;
    }
    strcpy(temp2, "recievedFile.txt");
    //strcpy(temp2, "recievedFile.txt");
    if(myRequest.requestFile(holder, temp2) == -1){
      perror("Client: Error: requestFile failure!!!");
      return -1;
    }
    myRequest.closeConnection();
  }
  for(int m = 1; m < argc; m++){
    memset(temp1, '\0',sizeof(temp1));
    memset(fName, '\0', sizeof(fName));
    memset(url, '\0',sizeof(url));
    int inputLen = strlen(argv[m]);
    if(inputLen <= 8){
      perror("Client: Error: URL format not recognized\n");
      break;
    }
    strcpy(holder, argv[m]);
    int i = 0;
    while(holder[i] != '/' && i<inputLen){
      i++;
    }
    if(i == inputLen){
      perror("Client: Error: URL format not recognized\n");
      break;
    }
    int j = i+2;
    int k = 0;
    while(holder[j] != ':' && j<inputLen){
      url[k] = holder[j];
      k++;
      j++;
    }
    if(j == inputLen){
      perror("Client: Error: URL format not recognized\n");
      break;
    }
    k=0;

    j++;
    while(holder[j] != '/' && j<inputLen){
      temp1[k] = holder[j];
      j++;
      k++;
    }
    if(j == inputLen){
      perror("Client: Error: URL format not recognized\n");
      break;
    }
    k = 0;
    prt = atoi(temp1);
    while(holder[j] != '\0'){
      fName[k] = holder[j];
      k++;
      j++;
    }
    HTTPrequest myRequest;
    strcpy(myRequest.targetURL,url);
    if(myRequest.openConnection(AF_INET, SOCK_STREAM, prt) == -1){
      perror("Client: Error: openConnection failure!!!");
      return -1;
    }
    strcpy(temp2, "recievedFile.txt");
    //strcpy(temp2, "recievedFile.txt");
    if(myRequest.requestFile(fName, temp2) == -1){
      perror("Client: Error: requestFile failure!!!");
      return -1;
    }
    myRequest.closeConnection();
    k = 0;
    i = 0;
    j = 0;
  }
  
  return 0;
}