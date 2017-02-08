/*
sws_server.c

implements a simple web server using the UDP

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

#define TRUE 1
#define FALSE 0

int sock;
char * port;
char * directory;
struct sockaddr_in sa;

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
	strcpy(dst, src);
	printf("dst: %s", dst);

	while(isspace(dst[strlen(dst) - 1]))
	{
		dst[strlen(dst) - 1] = '\0';
		printf("%s - ", dst);
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

	if(strncmp(version, "HTTP/1.0", 8) != 0)
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
		printf("Incorrect number of arguments.\n");
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

	printf("%s\n", directory);

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
	int opt = TRUE;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

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
   
	// FD_ZERO() clears out the called socks, so that it doesn't contain any file descriptors. 
	FD_ZERO( &read_fds );
	// FD_SET() adds the file descriptor "read_fds" to the fd_set, so that select() will return the character if a key is pressed 
	FD_SET( STDIN_FILENO, &read_fds );
	FD_SET( sock, &read_fds );

	printf("sws is running on UDP port %s and serving %s\n", port, directory);
	printf("press 'q' to quit ...\n");

	char readbuffer[10];

	while (1)
	{
		//select()
		fflush(STDIN_FILENO);
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
					printf("Recieved from stdin\n");
					read(STDIN_FILENO, readbuffer, 10);
					if(strncmp(readbuffer, "q", 1) == 0) //what was entered STARTS WITH q TODO: change to only q
					{
						printf("Goodbye!\n");
						close(sock);
						return EXIT_SUCCESS;
					}
					fflush(STDIN_FILENO);
				} else if(FD_ISSET(sock, &read_fds))
				{
					printf("Recieved through socket\n");
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

					parse_request(request, parseBuffer);

					if(!checkRequestMethod(parseBuffer[0]) || !checkURI(parseBuffer[1]) || !checkHTTPVersion(parseBuffer[2]))
					{
						printf("400\n");
					}
					else
					{
						//concat requested file onto served directory
						char dir[strlen(directory) + 1];
						strcpy(dir, directory);
						strcat(dir, parseBuffer[1]);

						if(!fileExists(dir))
						{
							printf("404\n");
						}
						else
						{
							//gather time string
							char timestring [80];
							getTimeString(timestring);

							//get client ip

							printf("%s ", timestring);
							printf("%s:%hu ", inet_ntoa(sa.sin_addr), sa.sin_port);
							char a[1024];
							strTrimInto(a, request);
							printf("%s %s %s;", parseBuffer[0], parseBuffer[1], a);
						}
					}


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