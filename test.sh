#!/bin/bash

set -o nounset

prog=./httpd
PORT=8080

function error() {
    echo "$@" >&2
    exit 1
}

# show version
echo './httpd 0.0.0' | cmp - <(./httpd -v 2>&1) || error "$LINENO"

# start server
$prog $PORT &

# Normal request
curl -s --head 127.0.0.1:${PORT}/hello.html | head -1 | grep 200 > /dev/null || error "$LINENO"

# Not Found
curl -s --head 127.0.0.1:${PORT}/not_found  | head -1 | grep 404 > /dev/null || error "$LINENO"

# Empty request
echo -n | telnet 127.0.0.1 ${PORT} 2>/dev/null | cmp - <<EOF
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
EOF
[[ $? != 0 ]] && error "$LINENO"

# stop server
kill %1

echo "=============================="
echo " All functional tests passed."
echo "=============================="
