//  NOTE: The following code was developed well before being committed on github, so some coding
//  choices maybe of dubious quality
//
//
//  Developed by Alec Barber
//  Jan/Feb 2018
//
//  SYNTAX
//  ./web-server [URL (localhost)] [port (4000)] [destination folder(current folder)]
//  Note: all inputs are optionial, defaults are shown in ()
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

class HTTPresponse{
  public:
    struct sockaddr_in servAddr, cliAddr;
    int sockHandle, cliHandle;

    char localURL[STR_LEN];
    char fileDirect[STR_LEN];

    HTTPresponse();

    int establishServer(int set1, int set2, int port);      // Calls all other methods and sets server up
    void closeHTTPResponse();                               // Closes server safely

  private:
    int setSocketAndAddr(int set1, int set2, int port);     // Set socket up and set addr info
    void setIP(int set1, int set2);                         // Converts the URL to an IP

    FILE* fileSearch(char* fileName);                       // Returns pointer to file if exists, a NULL pointer otherwise
    void fileParser(FILE* info);                            // Method to parse file into a buffer
    
    char localIP[STR_LEN]; 
    char hold[LNG_STR_LEN];                                 // Utility variable used to assist parser method, can ignore
};

inline HTTPresponse::HTTPresponse(){
  memset(localIP, '\0', sizeof(localIP));
  memset(localURL, '\0', sizeof(localURL));
  memset(hold, '\0', sizeof(hold));
  memset(fileDirect, '\0', sizeof(fileDirect));
}


inline int HTTPresponse::establishServer(int set1, int set2, int port){
  if(setSocketAndAddr(set1, set2, port) == -1){
    perror("Server: Error: setSocketAndAddr failed...\n");
    return -1;
  }

  //begin listening for incoming connections
  if(listen(sockHandle, LISTEN_BACKLOG) == -1){
    perror("Server: Error: Socket failed to set itself to listen\n");
    close(sockHandle);
    return -1;
  }
  std::cout<<"Server: running...waiting for connections...\n";
  //char fBuffer[LNG_STR_LEN];
  char respBuffer[LNG_STR_LEN];
  char recvBuffer[STR_LEN];
  char fname[STR_LEN];
  bool Success;

  for(;;){
    socklen_t addr_len = sizeof(cliAddr);
    int cliSockHandle = accept(sockHandle,(struct sockaddr*)&cliAddr,&(addr_len));
    std::cout<<"Server: Request recieved.\n";

    memset(recvBuffer,'\0',sizeof(recvBuffer));
    memset(respBuffer, '\0', sizeof(respBuffer));
    if(recv(cliSockHandle,recvBuffer,sizeof(recvBuffer),0) == -1){
      perror("Server: Error: Data not recieved using recv...\n");
      close(cliSockHandle);
      close(sockHandle);
      return -1;
    }

    //Current Version only supports one http 1.0 command GET
    if(recvBuffer[0] == 'G' && recvBuffer[1] == 'E' && recvBuffer[2] == 'T'){   //Recognized GET command
      //parse file name / directry from incoming stream
      memset(fname,'\0',sizeof(fname));
      int i = 4;
      while(recvBuffer[i] != ' '){
        fname[i-4] = recvBuffer[i];
        i++;
      }
      std::cout<<"Server: Recieved request for file :"<<fname<<std::endl;
      Success = true;
    }
    else{                                                                       //does not recogniz command
      perror("Server: Error: client command not recgnized.\n");
      strcpy(respBuffer, "HTTP/1.0 400 Bad request\r\n\r\n");
      if(send(cliSockHandle, respBuffer, sizeof(respBuffer),0) == -1){
        perror("Server: Error: Packet failed to send\n");
        close(cliSockHandle);
      }
      Success = false;
    }
    char dirAndName[STR_LEN];
    strcpy(dirAndName, fileDirect);
    strcat(dirAndName, fname);
    std::cout<<"Temp :"<<dirAndName<<std::endl;
    FILE* info = fileSearch(dirAndName);
    if(info != NULL && Success){                                                            //found file
      //strcpy(fBuffer, fileParser(info));
      strcpy(respBuffer, "HTTP/1.0 200 OK\r\n\r\n");
      fileParser(info);
      strcat(respBuffer, hold);
      if(send(cliSockHandle, respBuffer, sizeof(respBuffer),0) == -1){
        perror("Server: Error: Packet failed to send\n");
        close(cliSockHandle);
      }
    }
    else if(Success){                                                                       //did not find file
      perror("Server: Error: could not find file");
      strcpy(respBuffer, "HTTP/1.0 404 Not found\r\n\r\n");
      if(send(cliSockHandle, respBuffer, sizeof(respBuffer),0) == -1){
        perror("Server: Error: Packet failed to send\n");
        close(cliSockHandle);
      }
    }
  }

  return 0;
}

inline int HTTPresponse::setSocketAndAddr(int set1, int set2, int port){
  if(localURL[0] == '\0'){
    perror("Server: Error: URL not set");
    return -1;
  }
  setIP(set1, set2);
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);

  //assigning IP to port
  if(inet_aton(localIP, &servAddr.sin_addr) == -1){
    perror("Server: Error: IP Address failed to attach to socket.\n");
    return -1;
  }
  

  //create TCP IPv4 socket
  if((sockHandle = socket(set1, set2, 0)) == -1){
    perror("Server: Error: socket not created...\n");
    close(sockHandle);
    return -1;
  }

  //Bind socket to servAddr
  if(bind(sockHandle,(struct sockaddr*)&servAddr,sizeof(servAddr)) == -1){
    perror("Server: Error: Binding failed\n");
    close(sockHandle);
    return -1;
  }
  return 0;
}

inline FILE* HTTPresponse::fileSearch(char* fileName){
  if(fileName[strlen(fileName)-1] == '/')return NULL;
  FILE* fl = fopen(fileName, "r");
  return fl;
}

inline void HTTPresponse::fileParser(FILE* info){
  memset(hold, '\0',sizeof(hold));
  int i = 0;
  char lt = fgetc(info);
  while(lt != EOF){
    hold[i] = lt;
    i++;
    lt = fgetc(info);
  }
}

inline void HTTPresponse::setIP(int set1, int set2){
  struct addrinfo info;
  struct addrinfo* Pinfo;
  memset(&info, 0, sizeof(info));
  info.ai_family = set1;
  info.ai_socktype = set2;
  if(getaddrinfo(localURL, "80", &info, &Pinfo) != 0){
    perror("Client: Error: URLToIP Failed\n");
    exit(1);
  }
  struct sockaddr_in* IPv4 = (struct sockaddr_in*)Pinfo->ai_addr;
  inet_ntop(info.ai_family,&(IPv4->sin_addr),localIP,sizeof(localIP));
}

inline void HTTPresponse::closeHTTPResponse(){
  close(sockHandle);
  close(cliHandle);
}

int main(int argc, char *argv[]){
  int port;
  HTTPresponse myRequest;
  
  switch(argc){                                                                         //Test inputs and test accordingly
    case 1:   strcpy(myRequest.localURL,"localhost");
              port = 4000;
              strcpy(myRequest.fileDirect, ".");
              break;
    
    case 2:   strcpy(myRequest.localURL, argv[1]);
              port = 4000;
              strcpy(myRequest.fileDirect, ".");
              break;

    case 3:   strcpy(myRequest.localURL, argv[1]);
              port = std::stoi(argv[2]);
              strcpy(myRequest.fileDirect, ".");
              break;

    case 4:   strcpy(myRequest.localURL, argv[1]);
              port = std::stoi(argv[2]);
              strcpy(myRequest.fileDirect, argv[3]);
              break;
    
    default:  perror("Error: Too many arguments.");
              return -1;
  };

  if(myRequest.establishServer(AF_INET, SOCK_STREAM, port) == -1){
    perror("Server: Error: establishServer failed\n");
  }
  myRequest.closeHTTPResponse();
  return 0;
}
