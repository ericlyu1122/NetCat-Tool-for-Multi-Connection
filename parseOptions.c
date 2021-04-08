#include "commonProto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


// This is the start of the code to parse the command line options. It should be
// fairly complete but hasn't been extensively tested.

int parseOptions(int argc, char * argv[], struct commandOptions * co) {

  // First set the command options structure to known values

  co->option_k = 0;
  co->option_l = 0;
  co->option_v = 0;
  co->option_r = 0;
  co->option_p = 0;
  co->source_port = 0;
  co->timeout = 0;
  co->hostname = NULL;
  co->port = 0;
  
  int i;
  // iterate over each argument and classify it and
  // store information in the co structure;
  
  for (i = 1; i < argc; i++) {
      // Update last two tag
    // This next line is for illustraction purposes only and needs to be removed
    // once things are working 
//    fprintf(stderr, "Remove before submitting Arg %d is: %s\n", i, argv[i]);
    // Check for the various options

    if ((strcmp(argv[i], K_OPTION) == 0)) {
      co->option_k = 1;
    } else if ((strcmp(argv[i], L_OPTION) == 0)) {
      co->option_l = 1;
    } else if ((strcmp(argv[i], V_OPTION) ==0 )) {
      co->option_v = 1;
    } else if ((strcmp(argv[i], R_OPTION) == 0 )){
      co->option_r = 1;
    } else if ((strcmp(argv[i], P_OPTION) == 0)) {
      // got a port match, check next argument for port number
      i++;
      if (i >= argc) {
	// not enough arguments
	return PARSE_ERROR;
      } else { // extract port number
	// See man page for strtoul() as to why
	// we check for errors by examining errno
	errno = 0;
	char *parseEnd;
	co->source_port = strtoul(argv[i], &parseEnd, 10);
	if (errno != 0 || parseEnd == argv[i]) {
	  return PARSE_ERROR;
	} 
	co->option_p = 1;
      }
    } else if ((strcmp(argv[i], W_OPTION) == 0)) {
      // got a W match, check next argument for timeout value
      i++;
      if (i >= argc) {
	// not enough arguments
	return PARSE_ERROR;
      } else { // extract timeout value
	// See man page for strtoul() as to why
	// we check for errors by examining errno, see err
	errno = 0;
	char *parseEnd = NULL;
	co->timeout = strtoul(argv[i], &parseEnd, 10);
	if (errno != 0 || argv[i] == parseEnd) {
	  return PARSE_ERROR;
	}
	co->option_w = 1;
      }

    } else {
      // Things are tricker - If there is only one argument left then it is a port number.
      // if there are two arguments left then it is a hostname and port number and
      // if there are more arguments left then there are too many paramters on the line. 
      // and if there are more parameters on the line then this is a bug. 
      int argsLeft = argc - i;
      // Host name && port number are required
      // if (argsLeft != 2) return PARSE_TOOMANY_ARGS;

      if (argsLeft == 2) { //  hostName 
	co->hostname = argv[i];
	i++;
	argsLeft--;
      }

      if (argsLeft == 1) { // port
	errno = 0;
	char *parseEnd = NULL;
	co->port = strtoul(argv[i], &parseEnd, 10);
	if (errno != 0 || parseEnd == argv[i]) {
	  co->port = 0;
	  return PARSE_ERROR;
	}	
      }
    }
  }
  // At this point all the command line arguments have been parsed but they
  // haven't been checked for validity. It might make sense to check things
  // like port numbers to verify that they are in the valid range before
  // returning success and that the options don't contradict each other
  // It is up to you to decide how you want to proceed.
  return PARSE_OK;
}
