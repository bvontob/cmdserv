#!/bin/sh

PORT=12346

if which netcat >/dev/null 2>&1
then
    NETCAT=netcat
elif which nc >/dev/null 2>&1
then
    NETCAT=nc
else
    echo "Couldn't find netcat or equivalent" >&2
    exit 1
fi

rm -f t/test_cmdserv.stdout t/test_cmdserv.stderr t/test_cmdserv.conn

`which valgrind >/dev/null && echo "valgrind -q --error-exitcode=99"` \
    ./t/test_cmdserv \
    <  /dev/null \
    1> t/test_cmdserv.stdout \
    2> t/test_cmdserv.stderr &

SERVER_PID=$!

sleep 4

echo "-- 1st --" >> t/test_cmdserv.conn
printf "value get\r\nvalue set testcase\r\nvalue get\r\nexit\r\n" \
    | $NETCAT -p10001 -q5 localhost $PORT \
    >> t/test_cmdserv.conn
echo "-- 2nd --" >> t/test_cmdserv.conn
printf "value get\r\nserver shutdown\r\n" \
    | $NETCAT -p10002 -q5 localhost $PORT \
    >> t/test_cmdserv.conn

wait $SERVER_PID || exit $?

diff -u t/test_cmdserv.conn.exp   t/test_cmdserv.conn   || exit $?
diff -u t/test_cmdserv.stderr.exp t/test_cmdserv.stderr || exit $?
diff -u t/test_cmdserv.stdout.exp t/test_cmdserv.stdout || exit $?

rm -f t/test_cmdserv.stdout t/test_cmdserv.stderr t/test_cmdserv.conn
