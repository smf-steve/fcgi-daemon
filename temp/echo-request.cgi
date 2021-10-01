#! /bin/bash

# A simple CGI script that echos the Request Header

# Basic Request via the HTTP Protocol
#
# METHOD URI?QUERY PROTOCOL
# server: HOST
# <other request headers>
# <blank line>
# <optional body>


# Basic CGI Response

# Emit response headers
echo "X-function: Echoing Request"
echo "Content-type: text/plain"

# Emit blank line
echo ""

# Emit body


# If there is a Query String,
# emit the "&" separated components onto a separate line.
if [ -n "${QUERY_STRING}" ] ; then 
    IFS="&" read -a pairs <<< "${QUERY_STRING}"
    for kp in "${pairs[@]}" ; do
      echo ${kp}
    done   
fi

# Echo the HTTP request body
while read _line ; do
  echo ${_line} 
done 
echo $_line


exit 0

