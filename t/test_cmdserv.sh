#!/bin/sh

CMDSERV_PORT=12346
CMDSERV_HOST=::1

TEST_OUT_CLIENT=t/test_cmdserv.conn
TEST_OUT_STDERR=t/test_cmdserv.stderr
TEST_OUT_STDOUT=t/test_cmdserv.stdout

TEST_OUT_ALL="$TEST_OUT_CLIENT $TEST_OUT_STDERR $TEST_OUT_STDOUT"

STATUS=0

# TO DO: Instead of using netcat/nc, we should use our own tool based
#        on clientlib.h/.c
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

# TO DO: Let test cases have names instead of numbers. We can still
#        do the numbering ourselves (also needed to control the source
#        ports)
__TESTCASE__ () {
    sleep 1 # Not nice. But to make sure previous output has been written.

    if ! ps $SERVER_PID >/dev/null 2>&1
    then
        echo "Test server seems to have crashed/ended unexpectedly."
        wait $SERVER_PID
        exit $?
    fi

    echo "-- TESTCASE $1 --" \
        | tee -ai \
        $TEST_OUT_CLIENT \
        $TEST_OUT_STDERR \
        $TEST_OUT_STDOUT
    SOURCE_PORT="$1""0001"
}

# Prepare empty test output files, so we can use append mode everywhere
# and be sure that the markers and the real output get interleaved
rm -f $TEST_OUT_ALL
touch $TEST_OUT_ALL

# Start the server to test with, possibly using valgrind
# TO DO: Make valgrind use also configurable.
`which valgrind >/dev/null && echo "valgrind -q --error-exitcode=99 --track-origins=yes"` \
    ./t/test_cmdserv \
    <  /dev/null \
    1>> t/test_cmdserv.stdout \
    2>> t/test_cmdserv.stderr &

SERVER_PID=$!

echo "Waiting for test server to start up..."
while ! grep "ready for connections" t/test_cmdserv.stderr 2>/dev/null
do
    sleep 1
    grep "failed cmdserv_start" t/test_cmdserv.stderr 2>/dev/null
    if ! ps $SERVER_PID >/dev/null 2>&1
    then
        echo "Test server did not start up properly."
        wait $SERVER_PID
        exit $?
    fi
done


################################################################################
# Test cases
################################################################################

# TO DO: Add time outs for all connections to server, so we do not hang after
#        a server crash.
# TO DO: Check for exit codes of test programs and report them as test failures.
# TO DO: netcat's -q option is not supported on many systems: "after EOF on
#        stdin, wait the specified number of seconds and then quit. If seconds
#        is negative, wait forever." One of the reasons we should switch to our
#        own tool.

__TESTCASE__ 1

printf "value get\r\nvalue set testcase\r\nvalue get\r\nexit\r\n" \
    | $NETCAT -p $SOURCE_PORT -q5 $CMDSERV_HOST $CMDSERV_PORT \
    >> t/test_cmdserv.conn


__TESTCASE__ 2

for SOURCE_PORT in 20011 20021
do
    t/too-many-connections $SOURCE_PORT $CMDSERV_HOST $CMDSERV_PORT
done >> t/test_cmdserv.conn


__TESTCASE__ 3

printf "timeout\r\ntimeout 1\r\n" \
    | $NETCAT -p $SOURCE_PORT -q5 $CMDSERV_HOST $CMDSERV_PORT \
    >> t/test_cmdserv.conn


__TESTCASE__ 4

printf "a b c d e f g h i j k l m n o p q r s t u v w x y z\r\nexit\r\n" \
    | $NETCAT -p $SOURCE_PORT -q5 $CMDSERV_HOST $CMDSERV_PORT \
    >> t/test_cmdserv.conn


__TESTCASE__ 5

printf "value get\r\server status\r\nvalue get\r\n" \
    | t/close-no-read $SOURCE_PORT $CMDSERV_HOST $CMDSERV_PORT


__TESTCASE__ 6

printf "value get\r\nparse This is a \"nice command!\"\r\nserver shutdown\r\n" \
    | $NETCAT -p $SOURCE_PORT -q5 $CMDSERV_HOST $CMDSERV_PORT \
    >> t/test_cmdserv.conn

# TO DO: Add more test cases based on gcov output.

# TO DO: Abort after a short timeout, but continue with diffs
wait $SERVER_PID || exit $?

for OUTFILE in test_cmdserv.conn test_cmdserv.stderr test_cmdserv.stdout
do
    diff -u t/$OUTFILE.exp t/$OUTFILE
    DIFFSTATUS=$?
    if [ $DIFFSTATUS -eq 0 ]
    then
        rm t/$OUTFILE
    else
        STATUS=$DIFFSTATUS
    fi
done

exit $STATUS
