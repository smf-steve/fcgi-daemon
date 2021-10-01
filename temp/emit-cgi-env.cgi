#! /bin/bash

# A simple CGI script the emits all fo the CGI variables


# Basic Request via the HTTP Protocol
#
# METHOD URI?QUERY PROTOCOL
# server: HOST
# <other request headers>
# <blank line>
# <optional body>

# Basic CGI Response

# Emit response headers
echo "X-function: Emiting CGI variables"
echo "Content-type: text/plain"


# Emit blank line
echo ""

# Emit body

echo "# CGI Defined Environment Variables"
echo
echo GATEWAY_INTERFACE: ${GATEWAY_INTERFACE}
echo
echo REQUEST_METHOD: ${REQUEST_METHOD}      
echo REQUEST_URI: ${REQUEST_URI}            
echo PATH_INFO:${PATH_INFO}                 
echo QUERY_STRING: ${QUERY_STRING}          
echo SERVER_PROTOCOL: ${SERVER_PROTOCOL}    
echo HTTP_HOST: ${HTTP_HOST}                
echo
echo CONTENT_TYPE: ${CONTENT_TYPE}          
echo CONTENT_LENGTH: ${CONTENT_LENGTH}      
echo
echo SERVER_NAME: ${SERVER_NAME}            
echo SERVER_PORT: ${SERVER_PORT}            
echo
echo SCRIPT_FILENAME: ${SCRIPT_FILENAME}    
echo SCRIPT_NAME: ${SCRIPT_NAME} 	     
echo
echo HTTP_USER_AGENT: ${HTTP_USER_AGENT}    
echo
if [ -z "${DOCKER_CGI_TEST}" ] ; then
    echo "# An Envionmnet variable inserted to test the docker-cgi project"
    echo DOCKER_CGI_TEST: ${DOCKER_CGI_TEST}
fi



