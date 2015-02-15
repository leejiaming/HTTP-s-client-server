/*
** mp0client.c -- mp0 assignment--  jli100
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "connect_tls.h"
#include <arpa/inet.h>


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* strnstr_newline(const char* orig,int len){
		
	char * new_line = "\r\n\r\n";
	int i = 0;
	int newline_len = 4;
	int found = 1;
	char* head = NULL;
	for(;i<len;i++){
		
	    if(i+3 <len){
		found = strncmp((char*)orig+i,new_line,4);
	      if(found == 0){
		  head = (char*)orig+i;
		  return head;

		}
	      else
		continue;

	    }
	    else
		return head;

	}

}

int main(int argc ,char* argv[]){

	int status;
	struct addrinfo hints;
	struct addrinfo *rest, *funcAddr;
	char s[INET6_ADDRSTRLEN];
	memset(&hints,0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if(argc != 2){
		fprintf(stderr,"NEED HOST NAME\n");
		exit(1);
	}
	
	const char* request = argv[1];
	printf("The user request is %s\n", request);
	char* http_request = strstr(request,"http://");
	char* https_request = strstr(request,"https://");
	char* parsed_request;
	if(http_request == NULL && https_request== NULL){
		fprintf(stderr,"User must enter a valid url\n");
		exit(1);	

	}
	char* port = NULL;
	int head = 0 ;
	int tail = 0; //used to identify the port and file to be requested 
	int i = 0;
	char* filename ;

	if(http_request != NULL)
		parsed_request = (char*)request + 7;

	if(https_request != NULL)
		parsed_request = (char*)request + 8;
	
	
	char * parsed_request_sec = malloc(strlen(parsed_request)+2);
	if(strstr(parsed_request,"/")== NULL){
	    sprintf(parsed_request_sec,"%s/",parsed_request);
		parsed_request = parsed_request_sec;
	}
	printf("The parsed request is %s\n", parsed_request);
	for(;i<strlen(parsed_request);i++){
	
		char current = parsed_request[i];
		if(current == ':')
		  head = i;
		
		if(current == '/'){
		  tail = i;	
		  break;
		 }
	} 
	
	int port_len = tail-head -1 ; //if the user specify the port number;
	int filename_len = strlen(parsed_request) - tail;

	if(head != 0){  //get port
	 port = malloc(port_len+1);
	 strncpy(port,parsed_request+head+1,port_len);
	 port[port_len] = '\0';
	 printf("The port num is %s\n",port);	 	
	}
	
	filename = malloc(filename_len +1);  //get file name
	strncpy(filename,parsed_request+tail+1,filename_len);
	filename[filename_len] = '\0';

	printf("The file name is %s\n", filename);
	
	char* host;
	if(port == NULL){   //get the host name
	 host = malloc(tail + 1);
	 strncpy(host,parsed_request, tail);
	 host[tail] = '\0';
	}
	else{
	 host = malloc(head + 1);
	 strncpy(host,parsed_request, head);
	 host[head] = '\0';

	}
	printf("The host is %s\n",host);
	if(port == NULL && http_request!=NULL)
	   port = "80";  //this is a http_request with default port 80

	else if(port == NULL && https_request != NULL)
	   port = "443"; // this is a https_request with default port 443

	printf("CONNECT TO %s\n",port);
	int sockfd = NULL;
	TLS_Session* tls_client;
	if(http_request != NULL){
		printf("SET UP HTTP CONNECTION\n");
		if((status = getaddrinfo(host,port, &hints, &rest)) == -1){
	 	 	
			fprintf(stderr, "getaddrinfo:%s\n",gai_strerror(status));
			free(host);
		    if(head !=0)
			free(port);
			free(filename);
			free(parsed_request_sec);
			exit(1);

		}

		
		for(funcAddr = rest; funcAddr != NULL; funcAddr = rest->ai_next){
		
			if((sockfd = socket(funcAddr->ai_family, funcAddr->ai_socktype, funcAddr->ai_protocol)) == -1){
				//perror("CONTINUES CHECK FOR NEXT ADDRESS");
				continue;
			}
		
			if (connect(sockfd, funcAddr->ai_addr, funcAddr->ai_addrlen) == -1) {
				close(sockfd);
				perror("CLIENT CONNECT FAIL !!!");
				continue;
			}
		
			break;

		}
	
		if(funcAddr == NULL){
			perror("FAIL TO CONNECT !!");
			free(host);
		    if(head !=0)
			free(port);
			free(filename);
			free(parsed_request_sec);
			exit(1);
		}

		freeaddrinfo(rest); // all done with this structure
	}
	else{
		printf("SET UP HTTPS CONNECTION\n");
		clientInitTLS();
		tls_client = connectTLS(host,atoi(port));
		if(tls_client == NULL){
			perror("FAIL TO CONNECT");
			free(host);
		   if(head !=0)
			free(port);
			
			free(parsed_request_sec);
			free(filename);
			clientUninitTLS();
			exit(1);
		}

	}


	char req[strlen(filename)+18];
	sprintf(req, "GET /%s HTTP/1.0\r\n\r\n",filename);


	printf("REQUEST IS : %s",req);
	//req[strlen(filename)+17] = '\0';  why it is not working ???
	if(http_request!=NULL){
		if((send(sockfd,req, strlen(req),0))==-1){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(host);
		  if(head !=0)
			free(port);

			free(parsed_request_sec);
			free(filename);
			exit(1);
		
			}
	}
	else{
		if((sendTLS(tls_client,req,strlen(req)) < 0)){
			perror("FAIL TO THE SEND THE MESSAGE\n");
			free(host);
		   if(head !=0)
			free(port);
			free(filename);
			free(parsed_request_sec);
			shutdownTLS(tls_client);
			clientUninitTLS();
			exit(1);
		}
	

	}
	
	
	FILE* file = fopen("output", "wb"); //create an empty file ;
	int MAXSIZE = 1000;
	char rec_message[1000];
	int numBytes = 2;
	int total_len = 0;
	char *new_line = NULL;
	int foundHeader = 0;
	if(http_request != NULL){
		fd_set recvfd;
		FD_ZERO(&recvfd);
		FD_SET(sockfd,&recvfd);
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		int rv = select(sockfd+1,&recvfd,NULL,NULL, &tv);
		if(rv == -1)
		 	perror("select\n");

		else if(rv == 0)
			printf("Timeut occurred\n");


		else{
		       if(FD_ISSET(sockfd,&recvfd)){

				   while(numBytes > 0){

						memset(rec_message,0,1000);
						numBytes = recv(sockfd,rec_message,MAXSIZE,0);
						if(foundHeader == 1 && (new_line=strnstr_newline(rec_message,numBytes)) == NULL){
							fwrite(rec_message,1,numBytes,file);
						}

						if((new_line=strnstr_newline(rec_message,numBytes)) != NULL && foundHeader == 0){

							//printf("when found new_line: %s\n",rec_message);
							foundHeader = 1;	
							fwrite(new_line+4,1,numBytes-((new_line+4)-rec_message),file);
							//printf("NON_TEXT: %li\n",numBytes-((new_line+4)-rec_message));
							//printf("TEXT : %i\n",(int)strlen(new_line+4));
							}
						 }

					}
		 	 }
		
	}
	else{ 
	    while(numBytes > 0){
		memset(rec_message,0,1000);
		
		if((numBytes = recvTLS(tls_client,rec_message,MAXSIZE)) < 0){

		if(numBytes == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY)
			numBytes = 0;
		else{
			perror("recvice error--");
			printf("numBytes: %i\n",numBytes);
			free(host);
		    if(head !=0)
			free(port);

			free(filename);
			free(parsed_request_sec);
			shutdownTLS(tls_client);
			clientUninitTLS();
	   		exit(1);
			}
		}

		
				if(foundHeader == 1 && (new_line=strnstr_newline(rec_message,numBytes)) == NULL){
					fwrite(rec_message,1,numBytes,file);
					}
				if((new_line=strnstr_newline(rec_message,numBytes)) != NULL && foundHeader == 0){
						//printf("when found new_line: %s\n",rec_message);
					foundHeader = 1;	
					fwrite(new_line+4,1,numBytes-((new_line+4)-rec_message),file);

				}
	
	   }

	}
	fclose(file);
			
	close(sockfd);
	free(filename); //free all the allocated memory 
	if(head !=0)
	 free(port);

	free(host);
	free(parsed_request_sec);
	if(https_request != NULL){
	  shutdownTLS(tls_client);
	  clientUninitTLS(); 
	}
	return 1;
	

	

}


