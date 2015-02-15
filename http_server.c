/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXSIZE 3000 // maximum size of header

const char *HTTP_200_STRING = "HTTP/1.0 200 OK\r\n\r\n";
const char *HTTP_404_STRING = "HTTP/1.0 404 Not Found\r\n\r\n";
const char *HTTP_400_STRING = "HTTP/1.0 400 Bad Request\r\n\r\n";

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void process_request(const char* request, int client_fd){
	printf("process ID in process_request %i\n",getpid());	
	char *response_code ;
	// check for GET method from the cliend
	char* method = strstr(request, "GET ");
	if(strlen(method) == 0){
		//Bad requestr, server only handle GET for now 	
		response_code = (char*)HTTP_400_STRING;
		
		//*************might include more information to send back to client ********************************
		if((send(client_fd,response_code, strlen(response_code),0))==-1){
			perror("FAIL TO SEND THE MESSAGE\n");
			exit(1);
		
		}
		return;


	}
	
	// Find the length, minus "GET /"(4) and " HTTP/1.0"(9)...
	char* rec_request = strtok((char*)request, "\n");
	int len = strlen(rec_request) - 5 - 9;
	//printf("Length is %i\n", (int)strlen(rec_request));
	//printf("hearder : %s",rec_request);
	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 5, len);
	filename[len-1] = '\0';
	printf("File name is %s\n",filename);
	if(strlen(filename) == 0){
	  response_code = (char*)HTTP_400_STRING;
		if((send(client_fd,response_code, strlen(response_code),0))==-1){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			exit(1);
		
		}
		free(filename);
		return;
	
	}
	FILE *file = fopen(filename, "rb");
	//file not found 
	if(file == NULL){
		response_code = (char*)HTTP_404_STRING;
		if((send(client_fd,response_code, strlen(response_code),0))==-1){
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			exit(1);
		
		}
		free(filename);
		printf("FIle not found\n");
		return;


	}
		//calculate the length of file to send , then close the file ;
	     fseek(file, 0, SEEK_END);
	     int content_length = ftell(file);
	     fseek(file, 0, SEEK_SET);
	     char *content = malloc(sizeof(char)*content_length+1);
	     if(fread(content, content_length, 1, file) < 0){
				printf("FAIL TO READ FILE\n");
				response_code = (char*)HTTP_404_STRING;

				if((send(client_fd,response_code, strlen(response_code),0))==-1){
					perror("FAIL TO SEND THE MESSAGE\n");
					free(filename);
					free(content);
					exit(1);
				
				}
				free(content);
				free(filename);
				return;
			
				}
		//close the file stream, since contenet holds the file message.
	     fclose(file);
	     response_code = (char*)HTTP_200_STRING;
	     if((send(client_fd,response_code, strlen(response_code),0))==-1){ 
			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			free(content);
			exit(1);
		
		}

	
             int send_len = content_length;
	     int sent_len = 0;
	     int bytes ;
	     //printf("%s\n",content);

	    printf("------------SENDING FILE-------------\n-------------------------------------\n-------------------------------------\n-------------------------------------\n");

	    while(sent_len != send_len){
   	    if((bytes = send(client_fd, content+sent_len, content_length-sent_len, 0))==-1){

			perror("FAIL TO SEND THE MESSAGE\n");
			free(filename);
			free(content);
			exit(1);

		}
		  sent_len = sent_len + bytes;
	   
	     }
	    printf("------------FILE SENT-----------------\n\n");
		free(filename);
		free(content);
		return;
	
	
}


int main(int argc, char **argv)
{
	int sockfd, cli_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	if(argc !=2){
		printf("PORT NUM REQUIRE\n");
		exit(1);
	}
	char* PORT = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		//printf("Process ID: %i BEGIN ACCEPT\n",getpid());
		//printf("sockfd avaliable %i and its value is %i for process ID: %i\n",fcntl(sockfd,F_GETFD),sockfd,getpid());
		cli_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (cli_fd == -1) {
			perror("accept error:");
			sleep(20);
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s in process %i\n", s,getpid());

		if (!fork()) { // this is the child process
				//should child close sockfd ??????
			//printf("Child sockfd before close: %i, and its value is %i for process ID %i\n",fcntl(sockfd,F_GETFD),sockfd, getpid());
			close(sockfd);
			//printf("Child sockfd after close: %i, and its value is %i for process ID %i\n",fcntl(sockfd,F_GETFD),sockfd,getpid());
			int numbytes ;
			//printf("Child cli_fd before close: %i, and its value is %i for process ID %i\n",fcntl(cli_fd,F_GETFD),cli_fd,getpid());
			char buff[MAXSIZE];			
			if ((numbytes = recv(cli_fd, buff, MAXSIZE-1, 0)) == -1) {
				printf("%i", numbytes);
	   			 perror("recvice error");
	   			 break;
				}
			//printf("numBytes: %i\n",numbytes);
			char *request = malloc(numbytes + 1);
			strncpy(request, buff, numbytes);
			request[numbytes] = '\0';
			printf("%s\n",request);
			process_request(request, cli_fd);
			printf("**********CLOSING CONNECTION**********\n");
			free(request);

			shutdown(cli_fd,SHUT_RDWR);
	
			close(cli_fd);
			//printf("Child cli_fd after close: %i, and its value is %i for process ID %i\n",fcntl(cli_fd,F_GETFD),cli_fd,getpid());			
			exit(0);
			
			
		}
		//printf("%i\n",getpid());
		//sleep(1000);
		//printf("parent cli_fd before close: %i, and its value is %i for process ID: %i\n",fcntl(cli_fd,F_GETFD),cli_fd,getpid());
		//sleep(10);	
		close(cli_fd);//should we close cli_fd ????
		//printf("parent after before close: %i, and its value is %i for process ID: %i\n\n\n",fcntl(cli_fd,F_GETFD),cli_fd,getpid());	
	}

	return 0;
}

