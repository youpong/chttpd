#!/bin/bash

set -o nounset

prog=./httpd
PORT=8080

function error() {
    echo "$@" >&2
    exit 1
}


# start server
$prog $PORT &

# curl
curl -s --head 127.0.0.1:${PORT}/hello.html | head -1 | grep 200 > /dev/null || error "$LINENO"

# stop server
kill %1

echo "=============================="
echo " All functional tests passed."
echo "=============================="
