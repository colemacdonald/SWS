/*
sws_server.c

implements a simple web server using the UDP

Basis of file is from udp_server.c as shown in lab 2

*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() for socket */ 
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 1024

int sock;
char * port;
char * directory;
struct sockaddr_in sa;
char sendBuffer[BUFFER_SIZE];

void getTimeString(char * buffer)
{
	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (buffer, 80, "%b %d %T", timeinfo);
}

void strToUpper(char * str)
{
	char * s = str;
	while(*s)
	{
		*s = toupper((unsigned char) *s);
		s++;
	}
}

void strTrimInto(char * dst, char * src)
{
	strncpy(dst, src, strlen(src) - 4);

	while(isspace(dst[strlen(dst) - 1]))// || strcmp((char*)dst[strlen(dst) - 1], "\r") == 0 || strcmp((char*)dst[strlen(dst) - 1], "\n") == 0)
	{
		printf("iter\n");
		dst[strlen(dst) - 1] = '\0';
	}
}

int checkRequestMethod(char * method)
{
	strToUpper(method);
	if(strcmp(method, "GET") != 0)
	{
		return FALSE;
	}
	return TRUE;
}

int checkURI(char * filepath)
{
	if(filepath[0] != '/')
	{
		return FALSE;
	}
	return TRUE;
}

int fileExists(char * filepath)
{
	struct stat buf;
	int status = stat(filepath, &buf);

	if(status != 0)
	{
		return FALSE;
	}
	return TRUE;
}

int checkHTTPVersion(char * version)
{
	strToUpper(version);

	if(strcmp(version, "HTTP/1.0\r\n\r\n") != 0)
	{
		return FALSE;
	}
	return TRUE;
}

void parse_request(char * request_string, char ** buffer)
{
	const char s[2] = " ";

	char * token;

	token = strtok(request_string, s);

	int i = 0;

	while(token != NULL && i < 3)//buffer only has 3 spots
	{
		buffer[i] = token;
		token = strtok(NULL, s);
		i++;
	}
}

int directoryExists(char * directory)
{
	DIR* dir = opendir(directory);
	if( dir )
	{
		//directory does exist
		return TRUE;
	} 
	else if (ENOENT == errno)
	{
		//directory does not exist
		printf("Directory '%s' does not exist.\n", directory);
		return FALSE;
	}
	else
	{
		//failed for an unknown reason....
		printf("Checking directory failed for unknown reason.\n");
		return FALSE;
	}
}

int main( int argc, char ** argv )
{
	if( argc != 3)
	{
		printf("Incorrect number of arguments. Run as follows:\n ./sws <port> <directory>");
		return EXIT_FAILURE;
	}

	port = argv[1];
	directory = argv[2];
	strcat(port, "\0");
	if((char)directory[strlen(directory) - 1] == '/')
	{
		directory[strlen(directory) - 1] = '\0';
	} else 
	{
		strcat(directory, "\0");
	}

	if(!directoryExists(directory))
	{
		return EXIT_FAILURE;
	}

	//copied from udp_server.c
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock == -1)
	{
		close(sock);
		printf("socket could not be created - please try again\n");
		return EXIT_FAILURE;
	}

	//http://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	//struct sockaddr_in sa; 
	char buffer[1024];
	ssize_t recsize;
	socklen_t fromlen;

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons( atoi( port ) ); //convert to int
	fromlen = sizeof(sa);
	//end of copy

	if(bind(sock, (struct sockaddr *) &sa, sizeof sa) != 0)
	{
		printf("socket could not be bound\n");
		close(sock);
		return EXIT_FAILURE;
	}

	//prep fdset
	int select_result;
	fd_set read_fds;

	printf("sws is running on UDP port %s and serving %s\n", port, directory);
	printf("press 'q' to quit ...\n");

	while (1)
	{
		// FD_ZERO() clears out the called socks, so that it doesn't contain any file descriptors. 
		FD_ZERO( &read_fds );
		// FD_SET() adds the file descriptor "read_fds" to the fd_set, so that select() will return the character if a key is pressed 
		FD_SET( STDIN_FILENO, &read_fds );
		FD_SET( sock, &read_fds );

		//select()
		fflush(STDIN_FILENO);
		char readbuffer[1024];
		select_result = select( sock + 1, &read_fds, NULL, NULL, NULL );

		switch( select_result )
		{
			case -1:
				//error
				printf("Error in select (-1). Continuing.\n");
				break;
			case 0:
				//timeout -> should never happen as timeout is null
				printf("Error in select (0). Continuing.\n");
				break;
			case 1:
				//select returned properly
				if(FD_ISSET(STDIN_FILENO, &read_fds))
				{
					read(STDIN_FILENO, readbuffer, 1024);
					//fflush(STDIN_FILENO);
					if(strncmp(readbuffer, "q", 1) == 0) //what was entered STARTS WITH q TODO: change to only q
					{
						printf("Goodbye!\n");
						close(sock);
						return EXIT_SUCCESS;
					}
					else
					{
						printf("Unrecognized command.\n");
					}
					//fflush(STDIN_FILENO);
				} else if(FD_ISSET(sock, &read_fds))
				{
					fflush(STDIN_FILENO);
					ssize_t recsize;
					socklen_t fromlen = sizeof(sa);
					char request[4096];

					recsize = recvfrom(sock, (void*) request, sizeof request, 0, (struct sockaddr*)&sa, &fromlen);
					if(recsize == -1)
					{
						printf("Error occured.\n");
						continue;
					}

					char * parseBuffer[3]; //[0] == request method, [1] == request file, [2] == connection type
					parseBuffer[0] = char[1024];
					parseBuffer[1] = char[1024];
					parseBuffer[2] = char[1024];
					
					char tmp[strlen(request) + 1];

					strcpy(tmp, request);

					parse_request(tmp, parseBuffer);

					char response[1024];
					strcpy(response, "HTTP/1.0 ");

					FILE * fp;
					long int file_size;
					long int bytes_read;
					char dir[strlen(directory) + 1024];

					if(!checkRequestMethod(parseBuffer[0]) || !checkURI(parseBuffer[1]) || !checkHTTPVersion(parseBuffer[2]))
					{
						strcat(response, "400 Bad Request");
					}
					else
					{
						//concat requested file onto served directory
						strcpy(dir, directory);
						if(strcmp(parseBuffer[1], "/") == 0)
						{
							strcat(dir, "/index.html");
						}
						else
						{
							strcat(dir, parseBuffer[1]);
						}
						
						if(!fileExists(dir))
						{
							strcat(response, "404 Not Found");
						}
						else
						{
							//printf("dir: %s\n", dir);
							strcat(response, "200 OK");
							fp = fopen(dir, "r");
						}
					}
					//gather time string
					char timestring [80];
					getTimeString(timestring);

					printf("%s ", timestring);

					//ip and port
					printf("%s:%hu ", inet_ntoa(sa.sin_addr), sa.sin_port);

					//request string trimmed
					char requestTrimmed[1024];
					strTrimInto(requestTrimmed, request);
					printf("%s; ", requestTrimmed);
					printf("%s; ", response);

					//printf("\nreq: %s trimmed: %s\n", request, requestTrimmed);

					strcat(response, "\r\n\r\n");

					//sendto(sock, response, strlen(response), 0, (struct sockaddr*)&sa, sizeof sa);

					/*if(fp)
					{
						printf("%s\n", dir);

						fseek(fp, 0L, SEEK_END); //read to end
						file_size = ftell(fp);
						fseek(fp, 0L, SEEK_SET); //set pointer back to start

						char filebuffer[file_size];

						bytes_read = fread(filebuffer, sizeof(char), file_size, fp);

						//sending large file
						int index = 0;
						while(index < bytes_read)
						{
							sendto(sock, );
							index += BUFFER_SIZE;
						}
					}*/

					if(fp)
					{
						fclose(fp);
						fp = NULL;
					}
					printf("\n");
					//free(parseBuffer);

				}
				break;
			default:
				printf("Default select hit.");
				break;
				//wtf

		}
		//handle requests
		// use recvfrom() --> "normally used for connectionless-mode sockets because it permits the application
		// to retrieve the source address of the received data"
	}


	close(sock);
	return EXIT_SUCCESS;
}