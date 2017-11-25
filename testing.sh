#!/usr/bin/env bash

HOST='127.0.0.1'
PORT=8001
HTTP_REQ_DELIM='\r\n'
HTTP_REQ_END='\r\n\r\n'

set -x
# Normal Requests
(echo -en "GET / HTTP/1.1${HTTP_REQ_DELIM}Host: man7.org${HTTP_REQ_END}") | nc $HOST $PORT
set +x
