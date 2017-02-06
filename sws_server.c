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

#define TRUE 1
#define FALSE 0

void parse_request(char * request_string, char ** buffer)
{
	const char s[2] = " ";

	char * token;

	token = strtok(request_string, s);

	int i = 0;

	while(token != NULL)
	{
		printf("%s\n", token);
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

	char * port = argv[1];
	char * directory = argv[2];

	if(!directoryExists(directory))
	{
		return EXIT_FAILURE;
	}

	//copied from udp_server.c
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in sa; 
	char buffer[1024];
	ssize_t recsize;
	socklen_t fromlen;

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons( atoi( port ) ); //convert to int
	fromlen = sizeof(sa);
	//end of copy

	//http://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	if(bind(sock, (struct sockaddr *) &sa, sizeof sa) != 0)
	{
		printf("socket could not be bound");
		close(sock);
		return EXIT_FAILURE;
	}

	//prep fdset
	int select_result;
	fd_set read_fds;
   
	/* FD_ZERO() clears out the called socks, so that it doesn't contain any file descriptors. */
	FD_ZERO( &read_fds );
	/* FD_SET() adds the file descriptor "read_fds" to the fd_set, so that select() will return the character if a key is pressed */
	FD_SET( STDIN_FILENO, &read_fds );
	FD_SET( sock, &read_fds );

	printf("sws is running on UDP port %s and serving %s\n", port, directory);
	printf("press 'q' to quit ...\n");


	char * parseBuffer[3];

	char request[] = "GET / HTTP/1.0";

	parse_request(request, parseBuffer);
	printf("parsebuffer\n");

	int i;
	for(i = 0; i < 3; i++)
	{
		printf("%s\n", parseBuffer[i]);
	}

	char readbuffer[10];

	while (1)
	{
		//select()
		select_result = select( 2, &read_fds, NULL, NULL, NULL );
		
		//printf("%d\n", select_result);
		//printf("%zd\n", read(STDIN_FILENO, readbuffer, 10));

		//printf("readbuffer[0] = %s", &readbuffer[0]);
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