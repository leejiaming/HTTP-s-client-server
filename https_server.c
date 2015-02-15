#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include "connect_tls.h"

const char *HTTP_200_STRING = "HTTP/1.0 200 OK\r\n\r\n";
const char *HTTP_404_STRING = "HTTP/1.0 404 Not Found\r\n\r\n";
const char *HTTP_400_STRING = "HTTP/1.0 400 Bad Request\r\n\r\n";


void process_request(TLS_Session * tls_cli, const char* request){

	char *response_code = (char*)HTTP_400_STRING;
	// check for GET method from the cliend
	char* method = strstr(request, "GET ");
	if(strlen(method) == 0){
		//Bad requestr, server only handle GET for now 	
		response_code = (char*)HTTP_400_STRING;
		
		//*************might include more information to send back to client ********************************
		if((sendTLS(tls_cli,response_code, strlen(response_code)))==-1){
			perror("FAIL TO SEND THE MESSAGE\n");
			serverUninitTLS();
			exit(1);
		
		}
		serverUninitTLS();
		return;


	}

	// Find the length, minus "GET /"(4) and " HTTP/1.0"(9)...

		int	len = strlen(request) - 5 - 9;


	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 5, len);
	filename[len] = '\0';
	printf("File name is %s\n",filename);
	if(strlen(filename) == 0){
	  response_code = (char*)HTTP_400_STRING;
		if((sendTLS(tls_cli,response_code, strlen(response_code))) < 0){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			serverUninitTLS();
			exit(1);
		
		}
		free(filename);
		//serverUninitTLS();
		return;
	
	}
	FILE *file = fopen(filename, "rb");
	//file not found 
	printf("read 1\n");
	if(file == NULL){
		response_code = (char*)HTTP_404_STRING;
		if((sendTLS(tls_cli,response_code, strlen(response_code))) < 0){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			serverUninitTLS();
			exit(1);
		
		}
		free(filename);
		serverUninitTLS();
		printf("FIle not found\n");
		return;


	}
		//calculate the length of file to send , then close the file ;
	    //printf("REACH THIS LINE 1\n");
	    fseek(file, 0, SEEK_END);
	    int content_length = ftell(file);
	    fseek(file, 0, SEEK_SET);
	    char *content = malloc(sizeof(char)*content_length+1);
		printf("%i\n", content_length);
	    //printf("REACH THIS LINE 2\n");
	     if(fread(content, content_length, 1, file) < 0){
		printf("FAIL TO READ FILE\n");
		response_code = (char*)HTTP_404_STRING;
		if((sendTLS(tls_cli,response_code, strlen(response_code)))< 0){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			free(content);
			exit(1);
		
		}
		free(content);
		free(filename);
		serverUninitTLS();
		return;
	
		}
		//close the file stream, since contenet holds the file message.
	     fclose(file);
	     response_code = (char*)HTTP_200_STRING;
    /*
	     char send_message[strlen(response_code)+ content_length+1];
	     response_code = (char*)HTTP_200_STRING;
	     strncpy(send_message, response_code, strlen(response_code));
	     strncpy(send_message+strlen(response_code), content, content_length);
	     send_message[strlen(response_code)+content_length] = '\0';

	     int send_len = strlen(send_message);
	     int sent_len =  0;
	     int bytes ;
  */	

	    int send_len = strlen(response_code);
	    int sent_len = 0;
	    int bytes = 0;
	    printf("-----Sending Header-----\n");
	//the following is for send header 
	    while(sent_len != send_len){
		if((bytes = sendTLS(tls_cli,response_code+sent_len,strlen(response_code+sent_len))) < 0){
			perror("FAIL TO SEND THE HEADER\n");
			free(filename);
			free(content);	
			//free(send_message);	
			serverUninitTLS();
			exit(1);
		
		}
		 sent_len = sent_len + bytes;
	}
	    printf("-------Header Complete------\n");
	    send_len = content_length;
	    sent_len = 0;
	    bytes = 0;
	   printf("-------Sending Content------\n");
	    while(sent_len != send_len){
		if((bytes = sendTLS(tls_cli,content+sent_len,send_len-sent_len)) < 0){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			free(content);	
			//free(send_message);	
			serverUninitTLS();
			exit(1);
		
		}
		//printf("BYTES SENT IN CONTENTs:%i\n",bytes);
		 sent_len = sent_len + bytes;
	}
	 printf("-------Content Sent------\n");
		//free(send_message);	
		free(filename);
		free(content);	
	    	//printf("FILE SENT\n");
		return;
	
	


}


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

void sig_exit(int s){
	printf("\n-----------Clean up memory---------------\n");
	serverUninitTLS();
	exit(0);
		

}
int main(int argc, char** argv){

	if(argc != 2){
	  fprintf(stderr,"USER MUST ENTER PORT NUMBER!\n");
	  exit(1);
	
	}
	printf("REACH 1\n");
	if(serverInitTLS() < 0 ){
	  perror("FAIL TO INIT SERVER\n");
	  serverUninitTLS();
	  exit(1);
	}

	char* PORT = argv[1];
	int sockfd;
	if((sockfd = listenTCP(NULL,atoi(PORT))) < 0){
		perror("FAIL TO BIND THE IP AND PORT\n");
		serverUninitTLS();
		exit(1);
	}
	struct sigaction sa;
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		serverUninitTLS();
		exit(1);
	}
		
	signal(SIGINT,sig_exit);
	while(1){
	  printf("Waiting for connection\n");
	  int cli_fd = acceptTCP(sockfd);
	  if(cli_fd < 0 ){
		perror("INVALID FD\n");
		continue;

	  }
	  
	  TLS_Session* tls_client;
	  if(!fork()){	
		tls_client =  acceptTLS(cli_fd);	
		if(tls_client == NULL){
		   perror("INVALID CLIENT\n");
		   //continue;
		}
		
	 	int numBytes;
		char rec_message[1000];
		if((numBytes = recvTLS(tls_client,rec_message,1000)) < 0){
			perror("recvice error");
			//serverUninitTLS();
	   		exit(1);

		}
		
		rec_message[numBytes] = '\0';
		char* GET_header = strtok(rec_message,"\r\n");
		printf("HEADER IS : %s\n", GET_header);
		process_request(tls_client,GET_header);
		shutdownTLS(tls_client);
		serverUninitTLS();	
		exit(0);
		}
		//printf("parent : %i\n",a);
		close(cli_fd);
	     	//shutdownTLS(tls_client);	
		//serverUninitTLS();				

	}
			
		serverUninitTLS();
		//printf("DONE 5\n");
		
		return 0;
	  
}
