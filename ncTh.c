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
#include <sys/time.h>
#include "Thread.h"
#include "unistd.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#define TRUE 1
#define FALSE 0
#define TIMEOUT_MONITOR 1
#define SOCKET 2
#define STDIN_OUT 3
// change timeout default here INT_MAX
#define timeout_default 50
static long time_start = 0;
static int timeout_state = FALSE;
static int verbose = FALSE;
static int quit = FALSE;
static int server_fd;
static int cliend_fds[5] = { -1,-1,-1,-1,-1 };
static struct commandOptions cmdOps;


int create_socket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        fprintf(stderr, "failed to create socket.\n");
        exit(-1);
    }
    // Reuse the address
    int value = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0) {
        fprintf(stderr, "Failed to set the socket option\n");
        exit(-1);
    }

    if (ioctl(server_fd, FIONBIO, (char*)&value) < 0) {
        fprintf(stderr, "ioctl() failed\n");
        close(server_fd);
        exit(-1);
    }
    // Set blocking
    int opts = fcntl(server_fd, F_GETFL);
    if (opts < 0) {
        fprintf(stderr, "set blocking failed\n");
        exit(-1);
    }
    opts = opts & (~O_NONBLOCK);
    if (fcntl(server_fd, F_SETFL, opts) < 0) {
        fprintf(stderr, "fcntl set blocking failed\n");
        exit(-1);
    }

    return  server_fd;
}

static void * thread_function_timeout(void *timeout) {
    long timeout_monitor = (long) timeout;
    while(!quit) {
        struct timeval cur ;
        gettimeofday(&cur, 0);
        // TIME OUT CHANGES STATE
        if(cur.tv_sec - time_start > timeout_monitor) {
            timeout_state = TRUE;
            printf("Timeout Done\n");
            return NULL;
        }
        sleep(1);
    }
    return NULL;
}
static void * thread_function_socket(void * socket_fd) {
    char line[1024];
    while(TRUE) {
        if(quit) return NULL;
        memset(line, 0, 1024);
        if(read((int)(long)socket_fd, line, 1024) == -1) {
            fprintf(stderr, "Error in socket read.\n");
            return NULL;
        }
        // print what we read from socket
        printf("%s", line);
    }
    return NULL;

}

static void* thread_function_stdin_client(void* fd) {
    char line[1024];
    while (TRUE) {
        memset(line, 0, 1024);
        if (quit) return NULL;
        if (fgets(line, 1024, stdin) == NULL) return NULL;
        struct timeval time_stamp;
        gettimeofday(&time_stamp, 0);
        time_start = time_stamp.tv_sec;
        write((int)(long)fd, line, strlen(line));

    }
}


static void * thread_function_stdin(void * _ignore) {
     char line[1024];
     while(TRUE){
         memset(line, 0, 1024);
         if(quit) return NULL;
         if(fgets(line, 1024, stdin) == NULL) return NULL;
         struct timeval time_stamp;
         gettimeofday(&time_stamp, 0);
         time_start = time_stamp.tv_sec;
         for (int i = 0; i < 5; i++) {
             if (cliend_fds[i] != -1) {
                 write(cliend_fds[i], line, strlen(line));
             }
         }
     }
}

static void* thread_server(void* index) {
    char line[1024];
    while (TRUE ) {
        if (cmdOps.option_v)
            printf("try accept on %d \n", server_fd);
        int new_sd = accept(server_fd, NULL, NULL);
        cliend_fds[(int)(long)index] = new_sd;
        if (new_sd  == -1) {
            if (cmdOps.option_v)
                fprintf(stderr, "Error in accepting connection %d.\n", errno);
            // Retry
            continue;
        }
        if (cmdOps.option_v)
            printf("Connected: %d\n", new_sd);
        do {
            int read_return = read(new_sd, line, 1024);
            if (read_return == 0) {
                // End Of file
                break;
            }
            if (read_return == -1) {
                if (cmdOps.option_v)
                    fprintf(stderr, "Error in socket read.\n");
                break;
            }
            // Reset on receive
            struct timeval time_stamp;
            gettimeofday(&time_stamp, 0);
            time_start = time_stamp.tv_sec;
            for (int i = 0; i < 5; i++) {
                if (cliend_fds[i] != -1 && i != (int)(long)index) {
                    write(cliend_fds[i], line, read_return);
                }
            }
            write(STDOUT_FILENO, line, read_return);
        }while (TRUE);

        // close connection
        close(new_sd);
        cliend_fds[(int)(long)index] = -1;
        int terminate = TRUE;
        if (!cmdOps.option_k) {
            for (int i = 0; i < (cmdOps.option_r? 5: 1); i++) {
                if (cliend_fds[i] != -1) {
                    terminate = FALSE;
                    break;
                }
            }
        }
        if (terminate) {
            if (cmdOps.option_v)
                printf("Terminating\n");
            quit = TRUE;
        }

    }
    
}



int main(int argc, char **argv) {
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  

  int retVal = parseOptions(argc, argv, &cmdOps);
  if (cmdOps.option_v) verbose = TRUE;
  if (verbose) {
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
  unsigned int timeout;
  if (!cmdOps.option_w || cmdOps.timeout < 0) {
      // default connect 5 s
      timeout = timeout_default;
  }
  else {
      timeout = cmdOps.timeout;
  }

  if(cmdOps.option_l) {
      if (cmdOps.option_p) {
          fprintf(stderr, "-p used when -l specified.");
          exit(-1);
      }
      // create TCP socket IPv4
      server_fd = create_socket();
      // bind
      // sockaddr_in struct and mem allocate
      struct sockaddr_in address;
      bzero(&address, sizeof(struct sockaddr_in));
      address.sin_family = AF_INET;
      // address.sin_addr.s_addr = INADDR_ANY;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      address.sin_port = cmdOps.port > 65535 || cmdOps.port < 1024 ? 0 : htons(cmdOps.port);
      if (bind(server_fd, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) != 0) {
          fprintf(stderr, "failed to bind socket.");
          exit(EXIT_FAILURE);
      }
      // listen
      if (listen(server_fd, 1) < 0) {
          fprintf(stderr, "failed to listen for connection.");
          exit(EXIT_FAILURE);
      }
      if(cmdOps.option_v)
        printf("socket %d listening \n", server_fd);

      // get current time;
      struct timeval time_stamp;
      gettimeofday(&time_stamp, 0);
      time_start = time_stamp.tv_sec;
      struct Thread* timeout_monitor = createThread(thread_function_timeout, (void*)(long)timeout);
      runThread(timeout_monitor, NULL);
      struct Thread* std_in = createThread(thread_function_stdin, (void*)(long)timeout);
      runThread(std_in, NULL);
      struct Thread* server1 = createThread(thread_server, (void*)(long)0);
      runThread(server1, NULL);
      struct Thread* server2 = createThread(thread_server, (void*)(long)1);
      struct Thread* server3 = createThread(thread_server, (void*)(long)2);
      struct Thread* server4 = createThread(thread_server, (void*)(long)3);
      struct Thread* server5 = createThread(thread_server, (void*)(long)4);

      if (cmdOps.option_r) {
          runThread(server2, NULL);
          runThread(server3, NULL);
          runThread(server4, NULL);
          runThread(server5, NULL);
      }
      //if (timeout_state) quit = TRUE;
      // DONE OR TIMEOUT CLOSE
      joinThread(timeout_monitor, NULL);
      

      // timeout quit
      quit = TRUE;
      cancelThread(server1);
      if (cmdOps.option_r){
          cancelThread(server2);
          cancelThread(server3);
          cancelThread(server4);
          cancelThread(server5);
      }
      cancelThread(std_in);
      if (cmdOps.option_v)
        printf("Closing connection\n");
      // close and free the memory
      for (int i = 0; i < 5; i++) {
          if (cliend_fds[i] != -1) {
              if (cmdOps.option_v)
                  printf("Shutting down: %d\n", cliend_fds[i]);
              shutdown(cliend_fds[i], SHUT_RDWR);
              close(cliend_fds[i]);
          }
      }
      if(cmdOps.option_v)
        printf("shutting down\n");
      shutdown(server_fd, SHUT_RDWR);
      close(server_fd);
      free(timeout_monitor);
      free(server1);
      free(std_in);
      free(server2);
      free(server3);
      free(server4);
      free(server5);

  } else {
      if(cmdOps.option_p &&(!cmdOps.source_port || cmdOps.source_port>65535 || cmdOps.source_port<1024)) {
          fprintf(stderr, "No source port. AND Source port out of ranges\n");
          exit(-1);
      }
      // host name is empty
      if(cmdOps.hostname == NULL){
          fprintf(stderr, "No hostname provided\n");
          exit(-1);
      }
      if (cmdOps.option_k) {
          fprintf(stderr, "unknown k used while doing -p.\n");
          exit(EXIT_FAILURE);
      }
      if (cmdOps.hostname == NULL || cmdOps.port <= 0) {
          fprintf(stderr, "Error: host name & port failed. \n");
          exit(-1);
      }
      if (verbose)
          printf("Start connecting.\n");

      // initiate time out content

      struct timeval connection_timeout_val;
      connection_timeout_val.tv_sec = timeout;
      connection_timeout_val.tv_usec = 0;

      // initiate socket
      int socket_fd, socket_set_opt;
      if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          fprintf(stderr, "Failed opening socket, fd %d", socket_fd);
          exit(EXIT_FAILURE);
      }

      struct sockaddr_in local_host, ser_host;
      // only bind port with -p
      if (cmdOps.option_p && cmdOps.source_port >= 1024 && cmdOps.source_port <= 65535) {
          memset(&local_host, 0, sizeof local_host);
          local_host.sin_port = htons(cmdOps.source_port);
          local_host.sin_family = AF_INET;
          local_host.sin_addr.s_addr = htonl(INADDR_ANY);
          if (verbose)
              printf("Source port set: %d\n", cmdOps.source_port);
          if (bind(socket_fd, (struct sockaddr*)&local_host, sizeof local_host) == -1) {
              fprintf(stderr, "Bind Error.\n");
              exit(EXIT_FAILURE);
          }
      }
      



      // resolve domain
      struct hostent * domain_to_connect;
      if ((domain_to_connect = gethostbyname(cmdOps.hostname)) == NULL) {
          fprintf(stderr, "Get host by name Error.");
          exit(EXIT_FAILURE);
      }
      // set the server info
      memset((char *)&ser_host, 0, sizeof ser_host);
      int port = cmdOps.port >= 0 ? (int )cmdOps.port : 0;
      ser_host.sin_port = htons(port);
      ser_host.sin_family = AF_INET;
      memcpy((char *)&ser_host.sin_addr, (char *) domain_to_connect->h_addr, domain_to_connect->h_length);

      // set the socket
      if ((socket_set_opt = setsockopt(
              socket_fd,SOL_SOCKET, SO_SNDTIMEO,&connection_timeout_val, sizeof connection_timeout_val)) == -1) {
          fprintf(stderr, "Set Socket opt error\n");
          exit(EXIT_FAILURE);
      }
      // connect
      if(connect(socket_fd, (struct sockaddr * )&ser_host, sizeof ser_host) == -1) {
          if (errno == EADDRINUSE) {
              fprintf(stderr, "address in used \n");
              exit(EXIT_FAILURE);
          }
          if ( errno == EINPROGRESS) {
              fprintf(stderr, "time out. \n");
              exit(EXIT_FAILURE);
          }
          fprintf(stderr, "Connection failed. \n");
          exit(EXIT_FAILURE);
      }
      socklen_t local_host_len = sizeof local_host;
      if (getsockname(socket_fd, (struct sockaddr *)&local_host, &local_host_len ) < 0) {
          fprintf(stderr, "get host name Error.\n");
          exit(EXIT_FAILURE);
      }
      if(verbose) {
          printf("CONNECTION SUCCEED! \n"
                 "SOCKET FD: %d IP: %s, PORT: %d\n",
                 socket_fd, inet_ntoa(local_host.sin_addr), ntohs(local_host.sin_port));
      }
      // get current time;
      struct timeval time_stamp;
      gettimeofday(&time_stamp, 0);
      time_start = time_stamp.tv_sec;

      if (verbose)
          printf("current time is %ld\n Start Threading\n", time_stamp.tv_sec);
      // thread to monitor the timeout
      struct Thread * timeout_monitor = createThread(thread_function_timeout, (void *)(long)timeout);
      runThread(timeout_monitor, NULL);
      struct Thread * socket_thread = createThread(thread_function_socket, (void *)(long)socket_fd);
      runThread(socket_thread, NULL);
      struct Thread * stdin_thread = createThread(thread_function_stdin_client, (void *)(long)socket_fd);
      runThread(stdin_thread, NULL);

      if(timeout_state) quit = TRUE;
      // DONE OR TIMEOUT CLOSE
      joinThread(timeout_monitor, NULL);
      shutdown(socket_fd, SHUT_RD);
      // timeout quit
      quit = TRUE;
      joinThread(socket_thread, NULL);
      cancelThread(stdin_thread);
      joinThread(stdin_thread, NULL);

      // close and free the memory
      close(socket_fd);
      free(timeout_monitor);
      free(socket_thread);
      free(stdin_thread);

  }

}
