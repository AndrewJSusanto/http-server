#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <regex.h>
#include <fcntl.h>

#include <arpa/inet.h>

#define BUF_SIZE 2048 // all requests are <= 2048


// took bind.c and put it in one c file.
int create_listen_socket(uint16_t port) {
    signal(SIGPIPE, SIG_IGN);
    if (port == 0) {
        return -1;
    }
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        return -2;
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        return -3;
    }
    if (listen(listenfd, 500) < 0) {
        return -4;
    }
    return listenfd;
}



int checkURI(char* uri) { // type 1 is for PUT requests, type 0 for other
	int valid = 1;
	
	if(strlen(uri) > 19) {
		valid = 0;
	}
	if(uri[0] != '/') {
		valid = 0;
	}
	if((int)strlen(uri) > 1) { // if uri length > 1
		for(int i = 1; i < (int)strlen(uri); i++) { // /foo.txt valid
			// starts with /, rest can be [a-zA-Z0-9._]
			int check = isalnum(uri[i]);
			if(check == 0) {
				if(uri[i] == '.' || uri[i] == '_') {
				}
				else {
					valid = 0;
				}				
			}
			// else, valid remains true;
		}
	}
	
	return valid;
}

int validRequest(char* method, char* uri, char* input) { //returns length of request if true, returns negative if false
	char request[BUF_SIZE] = "";
	
	if(strcmp(method, "GET") == 0 && strcmp(method, "PUT") == 0 && strcmp(method, "HEAD") == 0) {
		return -1;
	}
	if(checkURI(uri) == 0) { // method is everything else
		return -2;
	}
	if(strcmp(input, "HTTP/1.1") != 0) {
		return -3;
	}
	
	sprintf(request, "%s %s %s\r\n", method, uri, input);
	printf("%s", request);
	return strlen(request); // two spaces and \r\n = 4
}

int checkHeader(char* buffer) {
	char* line = strtok(buffer, "\r\n");
	while(line != NULL) {
		for(int i = 0; i < (int)strlen(line); i++) {
			if(line[i] == ':' && isspace(line[i - 1]) != 0 && isspace(line[i + 1]) != 1) {
				return 0;
			}
		}
		line = strtok(NULL, "\r\n");
	}
	return 1;
}

int main(int argc, char* argv[]) {

	// argv[0] is the port number. to initiate the httpserver, we need to call ./httpserver <port>. 
	//create_listen_socket returns an fd depending on whether or not the port responded.
	
	int numargs = argc;
	printf("Num Arguments: %d\n", numargs);
	uint16_t port = (uint16_t)atoi(argv[1]);
	int socket = create_listen_socket(port);
	printf("%d\n", socket);
	if(socket <= 0) {	
		return -1;
	}
	
	while(1) { // for each client
		// for each buffer read... process in a certain way (depending on the input received by the client
		printf("Listening...\n");
		int socketfd = accept(socket, NULL, NULL);
		
		
		
		printf("Socket: %d\n", socketfd); // this value can be used to read/write to client as an fd.
		(socketfd > 0) ? printf("Socket accepted\n") : printf("Socket rejected\n");
		
		char buf[BUF_SIZE] = { '\0' };
		int sz = 0;
		char method[BUF_SIZE] = "", uri[BUF_SIZE] = "", input[BUF_SIZE] = "";
		char copy[BUF_SIZE] = "";
		char copy2[BUF_SIZE] = "";
		
		while((sz = recv(socketfd, buf, BUF_SIZE, 0)) > 0) { // read buffer into buf, sz is bytes read
			//parse data, determine what to do with data depending on command (get, put, head)
			
			
			sscanf(buf, "%s %s %s", method, uri, input);
			int start = validRequest(method, uri, input);
			char key[] = "Content-Length";
			strcpy(copy, &buf[start]);
			strcpy(copy2, &buf[start]);
			
			char* token = strtok(copy, "\r\n");
			while(token != NULL && strstr(token, key) == NULL) {
				token = strtok(NULL, "\r\n");
			}
			char* content_length = token;
	
			if(checkHeader(copy) == 0) {
				char bad[] = "Bad Header\n";
				send(socketfd, bad, sizeof(bad), 0);
				close(socketfd);
			}
			
			printf("Method, URI, Input processed: %s %s %s\n", method, uri, input);
			
			if(strcmp(method, "GET") == 0) {
				printf("Calling Get function...\n");
				char path[BUF_SIZE] = "";
				char contents[BUF_SIZE] = "";
				char response[BUF_SIZE] = "";
				int count = 0, sz;
				int fd = 0;
				strcpy(path, &uri[1]);
				printf("Path: %s\n", path);
				if(access(path, F_OK) != 0) {
					sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\nNot Found\r\n");
					send(socketfd, response, sizeof(response), 0);
					close(socketfd);
				}
				if((fd = open(path, O_RDONLY)) >= 0) {
					printf("FD: %d\n", fd);
					while((sz = read(fd, path, BUF_SIZE)) > 0) {
						count += sz;
						strcat(contents, path);
						memset(path, '\0', sz);
					}
					close(fd);
					
					sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s\r\n", count, contents);
					send(socketfd, response, sizeof(response), 0);
					close(socketfd);
				}
			}
			else if(strcmp(method, "PUT") == 0) { // here parse through headerfiles and msg body
				int fd = 0, count = 0;
				char path[BUF_SIZE] = "";
				char contents[BUF_SIZE] = "";
				char response[BUF_SIZE] = "";
				strcpy(path, &uri[1]);
				int created = 0, found = 0, i = 0;
				
				while(found == 0 && i < sz) {
					if(copy2[i] == '\r' && copy2[i + 1] == '\n') {
						printf("melody i love\n");
						if(copy2[i + 2] == '\r' && copy2[i + 3] == '\n') {
							printf("woaaah starts here\n");
							found = 1;
						}
					}
					i++;
				}
				
				if((fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644)) >= 0) {
					printf("FD: %d\n", fd);
					if((lseek(fd, 0, SEEK_END) == lseek(fd, 0, SEEK_SET)) && (lseek(fd, 0, SEEK_END != -1))) {
						created = 1;			
					}
						write(fd, token, sizeof(token));
					if(created == 1) { // if file was created;
						sprintf(response, "HTTP/1.1 200 OK\r\n%s\r\n\r\n%s\r\n", content_length, token);
					}
					else {
						while((sz = read(fd, path, BUF_SIZE)) > 0) {
							count += sz;
							strcat(contents, path);
							memset(path, '\0', sz);
						}
						close(fd);
						sprintf(response, "HTTP/1.1 201 Created\r\nContent-Length: %d\r\n\r\n%s\r\n", count, contents);
					}
					send(socketfd, response, sizeof(response), 0);
					close(socketfd);	
				}
			}
			else if(strcmp(method, "HEAD") == 0) {
				printf("Calling Head function...\n");
				char path[BUF_SIZE] = "";
				char response[BUF_SIZE] = "";
				int count = 0, sz;
				int fd = 0;
				strcpy(path, &uri[1]);
	
				if((fd = open(path, O_RDONLY)) >= 0) {
					printf("FD: %d\n", fd);
					while((sz = read(fd, path, BUF_SIZE)) > 0) {
						count += sz;
						memset(path, '\0', sz);
					}
					close(fd);
					
					sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", count);
					send(socketfd, response, sizeof(response), 0);
					close(socketfd);
				}
				else { // fd < 0
					printf("FD: %d\n", fd);
					int err = errno;
					if(err == ENOENT) {
						sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\nNot Found\r\n");
						send(socketfd, response, sizeof(response), 0);
						close(socketfd);
					}
					else {
						printf("File open failed: error code %d: %s\n", err, strerror(err));
					}
				}
			}
			else {
				printf("Invalid Request\n");
			}
			memset(buf, '\0', sz);
		}
		printf("Response received\n");
		
		close(socketfd);
	}


	
	return 0;
}


//GET / HTTP/1.1\r\n
//Host: localhost:1234\r\n
//User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:105.0) Gecko/20100101 Firefox/105.0\r\n
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n
//Accept-Language: en-US,en;q=0.5\r\n
//Accept-Encoding: gzip, deflate, br\r\n
//Connection: keep-alive\r\n
//Upgrade-Insecure-Requests: 1\r\n
//Sec-Fetch-Dest: document\r\n
//Sec-Fetch-Mode: navigate\r\n
//Sec-Fetch-Site: none\r\n
//Sec-Fetch-User: ?1\r\n
