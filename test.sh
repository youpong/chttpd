#!/bin/bash

./httpdctrl start
./client 'GET' | cmp - <(echo -n 200) || echo Error
./httpdctrl stop

# httpd echo back

# end
echo Okay.
