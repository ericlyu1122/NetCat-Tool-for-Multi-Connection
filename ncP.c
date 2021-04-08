#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "commonProto.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>

#define OPEN_MAX 5
#define FALSE 0
#define TRUE 1

int create_socket(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        fprintf(stderr, "failed to create socket.");
        exit(-1);
    }
    // Reuse the address
    int value = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0){
        fprintf(stderr, "Failed to set the socket option");
        exit(-1);
    }

    if (ioctl(server_fd, FIONBIO, (char *)&value) < 0){
        fprintf(stderr, "ioctl() failed");
        close(server_fd);
        exit(-1);
    }
    return  server_fd;
}

int main(int argc, char **argv) {

  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  int timeout;
  int rc;
  struct commandOptions cmdOps;
  int retVal = parseOptions(argc, argv, &cmdOps);
  if(cmdOps.option_v) {
      printf("Command parse outcome %d\n", retVal);
      printf("-k = %d\n", cmdOps.option_k);
      printf("-l = %d\n", cmdOps.option_l);
      printf("-v = %d\n", cmdOps.option_v);
      printf("-r = %d\n", cmdOps.option_r);
      printf("-p = %d\n", cmdOps.option_p);
      printf("-p port = %u\n", cmdOps.source_port);
      printf("-w  = %d\n", cmdOps.option_w);
      printf("Timeout value = %u\n", cmdOps.timeout);
      printf("Host to connect to = %s\n", cmdOps.hostname);
      printf("Port to connect to = %u\n", cmdOps.port);
  }

  if(cmdOps.option_k && !cmdOps.option_l)
      fprintf(stderr, "-k without -l");
  timeout = cmdOps.timeout == 0 ? (60*1000*60*24) : (int) cmdOps.timeout * 1000;

  // listen for an incoming connection
  // any -w ignore, any -p error
  if(cmdOps.option_l){
      if(cmdOps.option_p) {
          fprintf(stderr, "-p used when -l specified.");
          exit(-1);
      }
      // create TCP socket IPv4
      int server_close = FALSE;
      int server_fd = create_socket();
      
       // bind
      // sockaddr_in struct and mem allocate
      struct sockaddr_in address;
      bzero(&address, sizeof(struct sockaddr_in));
      address.sin_family = AF_INET;
      // address.sin_addr.s_addr = INADDR_ANY;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      address.sin_port = cmdOps.port>65535||cmdOps.port<1024 ? 0: htons(cmdOps.port);
      if (bind(server_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) != 0) {
          fprintf(stderr, "failed to bind socket.");
          exit(EXIT_FAILURE);
      }
      // listen
      if (listen(server_fd, 1) < 0) {
          fprintf(stderr, "failed to listen for connection.");
          exit(EXIT_FAILURE);
      }

      int count = 0;
      int size = cmdOps.option_r ? OPEN_MAX + 1 : 2;
      struct pollfd fds[size];
      memset(fds, 0, sizeof(fds));
      for (int i = 0; i < size; i++) {
          fds[i].fd = server_fd;
          fds[i].events = POLLIN;
      }
      fds[0].fd = STDIN_FILENO;
      fds[0].events = POLLIN;

      int current_size = 0, i, j;
      int end_server = FALSE, compress_array = FALSE;
      int new_sd = -1, len;
      int close_conn;
      char   buffer[1024];

      /* Loop waiting for incoming connects or for incoming data   */
      /* on any of the connected sockets.                          */
      do {
          if(cmdOps.option_v) {
              sleep(1);
              printf("Waiting on poll()...\n");
              for (int i = 0; i < size; i++) {
                  printf("%d, " ,fds[i].fd);
              }
              printf("\n");
          }
          rc = poll(fds, size, timeout);
          if (rc < 0){
//              if(cmdOps.option_v)
                fprintf(stderr, "  poll() failed");
              break;
          }

          // timeout ?
          if (rc == 0){
//              if(cmdOps.option_v)
                fprintf(stderr, "  poll() timed out.  End program.\n");
              break;
          }


          for (i = 0; i < size; i++){
              /* Loop through to find the descriptors that returned    */
              /* POLLIN and determine whether it's the listening       */
              /* or the active connection.                             */
              close_conn = FALSE;
              if (fds[i].revents & POLLHUP) {
                  printf("Connection %d initialized close", i);
                  close_conn = TRUE;
              }

              if (fds[i].fd == server_fd){
                  if(cmdOps.option_v) {
                      printf("  Listening socket is readable\n");
                  }

                  /* Accept all incoming connections that are            */
                  /* queued up on the listening socket before we         */
                  /* loop back and call poll again.*/
                  int state = FALSE;
                  if (!cmdOps.option_v) state = TRUE;
                  
                    /* Accept each incoming connection.*/
                    new_sd = accept(server_fd, NULL, NULL);
                    if (new_sd < 0){
                        if (errno != EWOULDBLOCK){
//                            if(cmdOps.option_v)
                            fprintf(stderr, "  accept() failed");
                            end_server = TRUE;
                        }
                        continue;
                    }

                    if(cmdOps.option_v)
                        printf("  New incoming connection - %d, assign to index %d\n", new_sd, i);
                    fds[i].fd = new_sd;
                    fds[i].events = POLLIN | POLLHUP;
                  
              }
                  /* This is not the listening socket, therefore an        */
                  /* existing connection must be readable                  */
              else{
                  if(cmdOps.option_v)
                    printf("  Descriptor %d is readable\n", fds[i].fd);
                  
                  /* Receive all incoming data on this socket            */
                  /* before we loop back and call poll again.            */
                  

                  while (1) {
                      if (i != 0) {
                          rc = recv(fds[i].fd, buffer, sizeof(buffer), MSG_DONTWAIT);
                      } else {
                          int c = poll(fds, 1, 0);
                          if (c <= 0){
                              // No data
                              break;
                          }
                          rc = read(STDIN_FILENO, buffer, 1);
                          // printf("User input: %c \n", buffer[0]);
                      }
                      
                      if (rc < 0){
                          if (errno != EWOULDBLOCK)
                          {
//                              if(cmdOps.option_v)
                                fprintf(stderr, "  recv() faile\n");
                              close_conn = TRUE;
                          }
                          break;
                      }

                      /* Check to see if the connection has been           */
                      /* closed by the client                              */
                      if (rc == 0)
                      {
                          if(cmdOps.option_v)
                            printf("  Connection closed\n");
                          close_conn = TRUE;
                          break;
                      }

                      /* Data was received                                 */
                      len = rc;
                      if(cmdOps.option_v)
                        printf("  %d bytes received\n", len);

                      /* Echo the data back to the client                  */
                      for (j =1; j < size; j++){
                          if(j != i && fds[j].fd!=server_fd) {
                              rc = send(fds[j].fd, buffer, len, 0);
                              if (rc < 0) {
                                  //ignore
                                  if(cmdOps.option_v)
                                    fprintf(stderr, "  send() failed\n");
                              }
                          }
                      }
                      if (i != 0) {
                          rc = write(STDOUT_FILENO, buffer, len);
                      }
                  } 

                  if (close_conn){
                      if(cmdOps.option_v)
                        printf("closing connection %d\n", i);
                      close(fds[i].fd);
                      fds[i].fd = server_fd;
                      fds[i].events = POLLIN;
                      compress_array = TRUE;
                      int terminate = TRUE;
                      if (!cmdOps.option_k) {
                          for (j = 1; j < size; j++)
                          {
                              if (fds[j].fd != -1 && fds[j].fd != server_fd) {
                                  terminate = FALSE;
                                  break;
                            }
                          }
                          if (terminate) {
                              if (cmdOps.option_v)
                                  printf("Terminating\n");
                              end_server = TRUE;
                          }
                      }
                  }


              }  /* End of existing connection is readable             */
          } /* End of loop through pollable descriptors              */
      } while (end_server == FALSE ); /* End of serving running.    */

      /* Clean up all of the sockets that are open                 */
      for (i = 0; i < size; i++) {
          if(fds[i].fd >= 0 && fds[i].fd != server_fd)
              close(fds[i].fd);
      }
      close(server_fd);
      exit(EXIT_SUCCESS);

  }else {
      // source port out of ranges
      if(cmdOps.option_p &&(!cmdOps.source_port || cmdOps.source_port>65535 || cmdOps.source_port<1024)) {
          fprintf(stderr, "No source port. AND Source port out of ranges\n");
          exit(-1);
      }
      // host name is empty
      if(cmdOps.hostname == NULL){
          fprintf(stderr, "No hostname provided\n");
          exit(-1);
      }
      char buffer[1024];
      int len, client_fd, ret;
      if((client_fd = socket(AF_INET, SOCK_STREAM,0)) < 0 ){
          fprintf(stderr, "Fail to connect socket. \n");
          exit(-1);
      }
      struct sockaddr_in client_address;
      bzero(&client_address, sizeof(struct sockaddr_in));
      client_address.sin_family = AF_INET;
      client_address.sin_addr.s_addr = htonl(INADDR_ANY);
      client_address.sin_port = htons(cmdOps.source_port);

      if (bind(client_fd, (struct sockaddr *)&client_address, sizeof (client_address)) < 0){
          fprintf(stderr, "Failed to bind socket");
          exit(-1);
     }

      struct sockaddr_in server_address;
      bzero(&server_address, sizeof(struct sockaddr_in));
      server_address.sin_family = AF_INET;
      struct hostent* hc = gethostbyname(cmdOps.hostname);
      if (hc == NULL) {
          fprintf(stderr, "error gethostbyname\n");
          exit(EXIT_FAILURE);
      }
      memcpy((char *)&server_address.sin_addr,(char *)hc->h_addr, hc->h_length);
      server_address.sin_port = htons(cmdOps.port);

      struct pollfd fds[2]; //Only the connection and stdin;
      memset(fds, 0 , sizeof(fds));
      // Set up the initial listening socket
      fds[0].fd = client_fd;
      fds[0].events = POLLOUT;
      if (timeout > 0) {
          struct timeval connect_timeout;
          connect_timeout.tv_sec = 0;
          connect_timeout.tv_usec = timeout;
          if ((ret = setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &connect_timeout, sizeof connect_timeout)) == -1) {
              fprintf(stderr, "Error: set socket \n");
              exit(-1);
          }
      } else {
          fprintf(stderr, "Error: timeout value \n");
          exit(-1);
      }
      // Connect to server
      int connect_status;
      if((connect_status = connect(client_fd, (struct sockaddr*) &server_address, sizeof (server_address))) < 0){
          if (errno != EINPROGRESS) {
              fprintf(stderr, "connecting timeout\n");
              exit(-1);
          }
          fprintf( stderr, "failed connecting\n");
          exit(-1);
      }
      if(cmdOps.option_v)
          printf("Wait to connect\n");
      // wait to be writeable;
      rc = poll(fds, 1, timeout);
      if(rc < 0){
          fprintf(stderr, "Cannnot connect to server with fd %d, Error %d", client_fd, errno);
          exit(-1);
      }
      if (rc == 0) {
//          if (cmdOps.option_v)
              fprintf(stderr, "  poll() timed out.  End program.\n");
          exit(-1);
      }
      fds[0].events = POLLIN;
      fds[1].fd = STDIN_FILENO;
      fds[1].events = POLLIN;
      // timeout val
      int disconnect = 0;
      /* Loop waiting for incoming connects or for incoming data   */
      /* on any of the connected sockets.                          */
      do {
          if (cmdOps.option_v) {
              printf("Waiting on poll()...\n");
          }
          rc = poll(fds, 2, timeout);
	  if (rc == 0){
	  	// printf("timeout\n");
          close(fds[0].fd);
		exit(0);
	  }
	  if(rc < 0){
	  	fprintf(stderr, "error poll\n");
		exit(-1);
 	  }
          for (int i = 0; i < 2; i++){
              /* Loop through to find the descriptors that returned    */
              /* POLLIN and determine whether it's the listening       */
              /* or the active connection.                             */
              if(fds[i].revents == 0)
		        continue;
              /* If revents is not POLLIN, it's an unexpected result,  */

              if((fds[i].revents & POLLIN) != POLLIN){
//                  if(cmdOps.option_v)
                      fprintf(stderr, "  Error! revents = %d\n", fds[i].revents);
                  disconnect = TRUE;
                  break;
              }
              if(cmdOps.option_v)
                  printf("  Descriptor %d is readable\n", fds[i].fd);
              disconnect = FALSE;
              do {
                  /* Receive data on this connection until the         */
                  /* recv fails with EWOULDBLOCK. If any other         */
                  /* failure occurs, we will close the                 */
                  /* connection.                                       */
                  if (i!= 1) {
                      rc = recv(fds[i].fd, buffer, sizeof(buffer), MSG_DONTWAIT);
                  } else {
                      // Make sure stdin has data
                      int c;
                      if ((c = poll(fds + 1, 1, 0)) <= 0) {
                          // No data
                          break;
                      }
                      rc = 1;
                      read(STDIN_FILENO, buffer, 1);
                  }
                  if (rc < 0) {
                      if (errno != EWOULDBLOCK) {
//                          if (cmdOps.option_v)
                              fprintf(stderr, "  recv() faile, error %d\n", errno);
                          disconnect = TRUE;
                      }
                      break;
                  }
                  /* Check to see if the connection has been           */
                  /* closed by the client                              */

                  if (rc == 0) {
                      if (cmdOps.option_v)
                          printf("  Connection closed\n");
                      disconnect = TRUE;
                      break;
                  }

                  len = rc;
                  if (cmdOps.option_v)
                      printf("  %d bytes received\n", len);

                  /* Echo the data back to the client                  */
                  for (int j = 0; j < 2; j++) {
                      if (j != i) {
                          if (j == 1)
                              rc = write(STDOUT_FILENO, buffer, len);
                          else
                              rc = send(fds[j].fd, buffer, len, 0);
                          if (rc < 0) {
//                              if (cmdOps.option_v)
                                  fprintf(stderr, "  send() failed\n");
                              disconnect = TRUE;
                              break;
                          }
                      }
                  }
              } while ( !disconnect);
              if(disconnect){
                  if(cmdOps.option_v)
                    printf("Connection terminated");
                  close(client_fd);
                  close(fds[1].fd);
                  close(fds[0].fd);
                  exit(0);
              }
          }
      } while (TRUE);

  }
}
