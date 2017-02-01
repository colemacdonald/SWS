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

int main( int argc, char ** argv )
{
	if( argc != 3)
	{
		printf("Incorrect number of arguments.\n");
		return EXIT_FAILURE;
	}

	char * port = argv[1];
	char * directory = argv[2];

	DIR* dir = opendir(directory);
	if( dir )
	{
		//directory does exist
	} 
	else if (ENOENT == errno)
	{
		//directory does not exist
		printf("Directory '%s' does not exist.\n", directory);
		return EXIT_FAILURE;
	}
	else
	{
		//failed for an unknown reason....
		printf("Checking directory failed for unknown reason.\n");
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
	printf("press q to quit ...\n");

	while (1)
	{
		//select()
		select_result = select( 1, &read_fds, NULL, NULL, NULL );
		//handle requests
		// use recvfrom() --> "normally used for connectionless-mode sockets because it permits the application
		// to retrieve the source address of the received data"
	}


	close(sock);
	return EXIT_SUCCESS;
}