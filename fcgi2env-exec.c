
/*******************************************************************************/
/*  The fcgi2env-exec programs:                                                */
/*     - reads an FCGI request from a client                                   */
/*     - prepares an environmment containg CGI variables                       */
/*     - exec-s the program provided as its only arguement.                    */
/*     - sends back to the result to the client.                               */
/*                                                                             */
/*******************************************************************************/
/* FCGI Protocol Definition: fcgi-spec.html                                    */
/*                                                                             */
/* This is a limited implementation of the FCGI protocol.                      */
/* The purpose of this implementation is for comparison to SCGI.               */
/* As such, only the components that are related to this comparison            */
/* have been implemented.                                                      */
/*                                                                             */
/* Specifically,                                                               */
/*    - Management record types are not supported                              */
/*    - Appplication records limited to the following:                         */
/*        o FCGI_BEGIN_REQUEST, FCGI_END_REQUEST                               */
/*        o FCGI_PARAMS                                                        */
/*        o FCGI_STDIN, FCGI_STDOUT                                            */
/*        o Note Implemented:                                                  */
/*            - STDERR, DATA, ABORT_REQUEST                                    */
/*    - END_REQUEST limited to REQUEST_COMPLETE                                */
/*    - RESPONDER is the only role                                             */
/*    - AUTHORIZER * FILTER roles NOT supported                                */
/*    - Strick adheres to the general communication flow                       */
/*    - CANT_MPX_CONN is presumed                                              */
/*        o i.e., the connection "id" is not validated                         */
/*                                                                             */
/*                                                                             */
/* Many of the declarations in this implementation have been taken from the    */
/*     FastCGI Specification, Mark R. Brown (see "fcgi-spec.html")             */
/*                                                                             */
/* We have tried to stay consist with the abstract C declaration and the       */
/* overall layout of the specification, whenever poossible. As the spec.       */
/* can be used as additional documentation for this implementation.            */
/* and then adapted for our implementation.                                    */
/*                                                                             */
/*******************************************************************************/

// Contraints that have not been implemented.

// A Responder performing an update, e.g. implementing a POST method,
// should compare the number of bytes received on FCGI_STDIN with
// CONTENT_LENGTH and abort the update if the two numbers are not
// equal.

// Outgoing Padding Lengths set to zero.
// We recommend that records be placed on boundaries that are multiples of eight bytes. The fixed-length portion of a FCGI_Record is eight bytes.


// flags & FCGI_KEEP_CONN: If zero, the application closes the
// connection after responding to this request. If not zero, the
// application does not close the connection after responding to this
// request; the Web server retains responsibility for the connection.

// There is no check to see if the child closes it's stdin abnormally.

// Assume that a name and value are always provided within the same packet.

// There is a bit of duplication in the code... creating subroutines might be in order.
// Note that number of subroutines are not that many.




#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <assert.h>

#include "fcgi-spec.h"

#define exit_error(b,v) if (b) exit(v);


#undef FCGI_LISTENSOCKET_FILENO    // In this implementation, we use STDIN_FILENO


// Implementation: Return Values
#define RETVAL_SUCCESS        (0)
#define RETVAL_PROTOCOL_ERROR (1)
#define RETVAL_ID_MISMATCH    (2)
#define RETVAL_UNABLE_TO_EXEC (3)
#define RETVAL_READ_WRITE_ERR (4)
#define RETVAL_MEMORY_ERR     (5)
#define RETVAL_TOO_MANY_ENVS  (6)
#define RETVAL_OTHER          (7)
/*                                                                    */
/**********************************************************************/

#define TRUE (0)
#define FALSE (! TRUE )


#define BYTE unsigned char
#define ZERO (0)
#define NONZERO (!0)

#define MAX_STDOUT_BUFFER    (0xFFFF)

#define MAX_ENV_COUNT 100
#define PROGRAM (argv[1])

#define SELF  (0)
#define child_stdin  pipe_to_child[0]
#define to_child     pipe_to_child[1]
#define from_child   pipe_to_parent[0]
#define child_stdout pipe_to_parent[1]






int main(int argc, char * argv[], char **envp) {
  
  int retval; // A generic variable used to obtain the return value of system routines

  int request_id;  // The Request ID for the associated FCGI request

  //  Some buffers to process the FCGI request
  FCGI_Header * buffer_header;
  BYTE * buffer_content;   int content_length=0; 
  BYTE * buffer_padding;   int padding_length=0;


  // A child CGI process is created to execute the target program */
  // This child process is
  //     created via a fork
  //     execle with an environment
  //     communicates with the parent via two pipes
  int child_pid = -1;
  
  BYTE * child_env[MAX_ENV_COUNT];

  int pipe_to_child[2];  // child(0) <-- parent 
  int pipe_to_parent[2];  // parent <-- child(1) 


  // Create buffer space to read the FCGI_Header, the Content, and the Padding.
  buffer_header  = (FCGI_Header *) malloc(sizeof(FCGI_Header));
  buffer_content = (BYTE *) malloc(FCGI_MAX_CONTENT_LEN);
  buffer_padding = (BYTE *) malloc(FCGI_MAX_PADDING_LEN);
  memset(buffer_padding, ZERO, FCGI_MAX_PADDING_LEN);


  // Create the pipes for communcation with the child.
  pipe(pipe_to_child);
  pipe(pipe_to_parent);
  

  
  /*******************************************************************************/
  /* Program Flow                                                                */
  /*    - Receive: {FCGI_BEGIN_REQUEST, id, {FCGI_RESPONDER, flags} }            */
  /*                                                                             */
  /*    - Receive: {FCGI_PARAMS, id, <string> }+                                 */
  /*    - Build:   Create the environment for the child process                  */
  /*                                                                             */
  /*    - Fork:    Create a child process to execute the CGI program             */
  /*                                                                             */
  /*    - Receive: {FCGI_STDIN, id, <string> }+                                  */
  /*    - Send:    <string>+ to child process                                    */
  /*                                                                             */
  /*    - Receive: STDOUT from child process                                     */
  /*    - Send:    {FCGI_STDOUT, id, <string> }+                                 */
  /*                                                                             */
  /*    - Wait:    Block until the child process returns                         */
  /*    - Send:    {FCGI_END_REQUEST, 1, {status, FCGI_REQUEST_COMPLETE}         */
  /*                                                                             */
  /*******************************************************************************/


  
  /*******************************************************************************/
  /*    - Receive: {FCGI_BEGIN_REQUEST, id, {RESPONDER, flags} }                 */
  /*******************************************************************************/
  {  

    retval = read(STDIN_FILENO, (BYTE *) buffer_header, sizeof(FCGI_Header));
    {
      // Validate the Request Header
      exit_error(buffer_header->version != FCGI_VERSION_1, RETVAL_PROTOCOL_ERROR);
      exit_error(buffer_header->type != FCGI_BEGIN_REQUEST, RETVAL_PROTOCOL_ERROR);
      
      request_id = (buffer_header->requestIdB1 <<8 ) | buffer_header->requestIdB0;

      exit_error(buffer_header->contentLengthB0 != sizeof(FCGI_BeginRequestBody), RETVAL_PROTOCOL_ERROR);
      
    }
    {
      // Read the Request Body */
      FCGI_BeginRequestBody * request_body;
      int role;
      
      retval = read(STDIN_FILENO, (BYTE *) request_body, sizeof(FCGI_BeginRequestBody) );
      role = (request_body->roleB1 <<8 ) | request_body->roleB0;

      exit_error(retval != sizeof(FCGI_BeginRequestBody), RETVAL_READ_WRITE_ERR);
      exit_error(role != FCGI_RESPONDER, RETVAL_PROTOCOL_ERROR);
      exit_error(ZERO != request_body->flags, RETVAL_PROTOCOL_ERROR);
    }
  }


  
  /*******************************************************************************/
  /*    - Receive: {FCGI_PARAMS, id, <string> }+                                 */
  /*    - Build:   Create the environment for the child process                  */
  /*******************************************************************************/
  {
    int env_count = 0;

    do  {
   
      // Read the Header
      retval = read(STDIN_FILENO, (BYTE *) buffer_header, sizeof(FCGI_Header) );
      {
	/* Validate the contents */
	exit_error(buffer_header->version != FCGI_VERSION_1, RETVAL_PROTOCOL_ERROR);
	exit_error(buffer_header->type    != FCGI_PARAMS, RETVAL_PROTOCOL_ERROR);
      
	exit_error(request_id != ((buffer_header->requestIdB1 << 8 ) | buffer_header->requestIdB0), RETVAL_ID_MISMATCH);
      
	content_length = buffer_header -> contentLengthB1 << 8 | buffer_header -> contentLengthB0;
	padding_length = buffer_header -> paddingLength;
      
      }

      // Read the contentData and paddingdata, while constructing the new environment
      if (content_length != 0 ) {
	BYTE * p;   // a walking pointer within the content buffer
	BYTE * end; // a mark pointer to the end of the content buffer

	int name_length;  BYTE * name;     // The name component of the Name-Value pair
	int value_length; BYTE * value;    // The value component of the Name-Value pair


	// read the contentData
	retval = read(STDIN_FILENO, (BYTE *) buffer_content, content_length);
	exit_error(retval != content_length, RETVAL_READ_WRITE_ERR);

	p = buffer_content;
	end = buffer_content + content_length;

	while (p < end) {

	  // Determine the correct FCGI_ NameVaulePair to use as our template.
	  // based upon the MSbit in p[0], and p[1] or p[4]
	  // 
	  // Review the file "fcgi.h" for additional information
	  {
	    unsigned char t1, t2;
	  
	    t1 = p[0] >> 7;
	    t2 = ((t1 == 0 ) ? p[1] : p[4] ) >> 7;

	    switch (t1 << 1 | t2 ) {
	    
	    case 0: /* 00 => FCGI_NameValuePair11 */
	      name_length = p[0];
	      value_length = p[1];
	      p = (p+2);
	      break;
	    
	    case 1: /* 01 => FCGI_NameValuePair14 */
	      name_length = p[0];
	      value_length = ((p[1] & 0x7f) << 24) + (p[2] << 16) + (p[3] << 8) + p[4];
	      p = (p+5);
	      break;
	    
	    case 2: /* 10 => FCGI_NameValuePair41 */
	      name_length =  ((p[0] & 0x7f) << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	      value_length = p[5];
	      p = (p+6);
	      break;
	    
	    case 3:  /* 11 => FCGI_NameValuePair44 */
	      name_length = ((p[0] & 0x7f) << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	      value_length = ((p[4] & 0x7f) << 24) + (p[5] << 16) + (p[6] << 8) + p[7];
	      p = (p+8);
	      break;
	    
	    default:
	      assert(FALSE);
	      exit_error(TRUE, RETVAL_PROTOCOL_ERROR);
	    }
	  }
	  name = p;
	  value = name + name_length;

	
	  // Process the Name-Value pair
	  {
	    BYTE * q;
	    child_env[env_count] = (BYTE *) malloc(name_length + 1 + value_length + 1);
	    q = child_env[env_count];
	
	    memmove( q, (char *) name, name_length); q += name_length;
	    (*q) = '=' ; q++;
	    memmove( q, (char *) value, value_length); q += value_length;
	    (*q) = '\0'; q++;
	  }

	  // Setup for the next Name-Value pair
	  env_count ++;
	  p = value + value_length;
	}
      }

      if (padding_length != 0 ) {
	retval = read(STDIN_FILENO, (BYTE *) buffer_content, padding_length);
	exit_error(retval != content_length, RETVAL_READ_WRITE_ERR);
      }
      
      /* A record of the form {PARAMS, id, ""} denotes end of PARAMS */
    } while (content_length != 0);
    child_env[env_count] = NULL;
    child_env[env_count+1] = (BYTE *) 0xDEADBEAF;    // To help with debugging
    
    assert(env_count < MAX_ENV_COUNT);
  }



  /*******************************************************************************/
  /*    - Fork: a child process to execute the CGI program                       */
  /*******************************************************************************/
  {
    child_pid = fork();
    if (child_pid == SELF ) {
      // This is the child process: setup communication 
      
      dup2(child_stdin, 0);    close(to_child);
      dup2(child_stdout, 1);   close(from_child);
      
      execle(PROGRAM, PROGRAM, (char *) NULL, child_env);
      exit(RETVAL_UNABLE_TO_EXEC);
    }
    close(child_stdin); close(child_stdout);
  }


  /*******************************************************************************/    
  /*    - Receive: {FCGI_STDIN, id, <string> }+                                  */
  /*    - Send: <string> + to child process                                      */
  /*******************************************************************************/    
  {
    do  {

      // Read the rest of the FCGI_Header
      {
	retval = read(STDIN_FILENO, (BYTE *) buffer_header, sizeof(FCGI_Header) );
	
	/* Validate the contents */
	exit_error(buffer_header->version != FCGI_VERSION_1, RETVAL_PROTOCOL_ERROR);
	exit_error(buffer_header->type    != FCGI_STDIN,     RETVAL_PROTOCOL_ERROR);
	
	exit_error(request_id != ((buffer_header->requestIdB1 << 8 ) | buffer_header->requestIdB0), RETVAL_ID_MISMATCH);
	
	content_length = buffer_header -> contentLengthB1 << 8 | buffer_header -> contentLengthB0;
	padding_length = buffer_header -> paddingLength;
	
      }

      // Read the content and send it to the child
      if (content_length != 0 ) {
	retval = read(STDIN_FILENO, (BYTE *) buffer_content, content_length);
	retval = write(to_child, (BYTE *) buffer_content, content_length);
      }

      // Read the padding
      if (padding_length != 0 ) {
	retval = read(STDIN_FILENO, (BYTE *) buffer_content, padding_length);
      }
      
      /* A record of the form {FCGI_STDIN, id, ""} denotes end of 'stdin' */
    } while (content_length != 0 );  
    close(to_child);
  }


  // Enhancement: Allow for a request_ID > 255
  exit_error(request_id >= 256, RETVAL_OTHER);

  /*******************************************************************************/    
  /*    - Receive: STDOUT from child process                                     */
  /*    - Send:    {FCGI_STDOUT, id, <string> }+                                 */
  /*******************************************************************************/
  {
    /* Prepare the FCGI header */
    buffer_header->version = FCGI_VERSION_1;
    buffer_header->type = FCGI_STDOUT;
    buffer_header->requestIdB1 = ZERO;
    buffer_header->requestIdB0 = request_id;
    buffer_header->contentLengthB1 = ZERO;
    buffer_header->contentLengthB0 = ZERO;   
    buffer_header->paddingLength = ZERO;
    buffer_header->reserved = ZERO;

    /* Copy Child's STDOUT to FCGI_STDOUT */
    do  {
      content_length = read(from_child, (BYTE *) buffer_content, MAX_STDOUT_BUFFER );

      padding_length = 0;        // Enhancement:  Calculate an appropriate padding_length
      
      buffer_header->contentLengthB1 = (unsigned char) (content_length & 0x00FF ) >> 8;
      buffer_header->contentLengthB0 = (unsigned char) (content_length & 0xFF00) ;

      buffer_header->paddingLength   = padding_length;

      
      retval  = write(STDOUT_FILENO, (BYTE *) buffer_header, sizeof(FCGI_Header));
      if (content_length != 0) {
	retval += write(STDOUT_FILENO, (BYTE *) buffer_content, content_length);
      }
      exit_error( retval != (sizeof(FCGI_Header) + content_length), RETVAL_READ_WRITE_ERR);

      if ( padding_length != 0 ) {
	retval = write(STDOUT_FILENO, buffer_padding, padding_length);
	exit_error ( retval != padding_length, RETVAL_READ_WRITE_ERR)
      }

    } while (content_length != 0);
    // All output from the child has been processed.
  }
		     



  /*******************************************************************************/    
  /*    - Wait:    Block until the child process returns                         */
  /*    - Send:    {FCGI_END_REQUEST, 1, {status, REQUEST_COMPLETE}              */
  /*******************************************************************************/    
  {
    int status = 0;

    // Enhancement: We assume that status < 256
    wait(&status);

    /* Prepare the FCGI header */
    buffer_header->version = FCGI_VERSION_1;
    buffer_header->type = FCGI_END_REQUEST;
    buffer_header->requestIdB1 = ZERO;
    buffer_header->requestIdB0 = request_id;
    buffer_header->contentLengthB1 = ZERO;
    buffer_header->contentLengthB0 = sizeof(FCGI_EndRequestBody);
    buffer_header->paddingLength = ZERO;
    buffer_header->reserved = ZERO;
      
    /* Prepare the EndRequest Body */
    // Use the already allocated buffer_content
    
    FCGI_EndRequestBody * buffer_body = (FCGI_EndRequestBody *) buffer_content;
      
    buffer_body->appStatusB3 = ZERO;
    buffer_body->appStatusB2 = ZERO;
    buffer_body->appStatusB1 = ZERO;
    buffer_body->appStatusB0 = (unsigned char) status;
    buffer_body->protocolStatus = FCGI_REQUEST_COMPLETE;
    buffer_body->reserved[0] = buffer_body->reserved[1] = buffer_body->reserved[2] = ZERO;
    

    retval  = write(STDOUT_FILENO, (BYTE *) buffer_header, sizeof(FCGI_Header) );
    retval += write(STDOUT_FILENO, (BYTE *) buffer_body, sizeof(FCGI_EndRequestBody) );
    exit_error( retval != sizeof(FCGI_Header) + sizeof(FCGI_EndRequestBody), RETVAL_READ_WRITE_ERR)

  }

  
  exit(0);
}


