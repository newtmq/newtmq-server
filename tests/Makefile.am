check_PROGRAMS = newt_test

newt_test_SOURCES = test.c \
                    connection.c \
                    client.c \
                    daemon.c \
                    config.c \
                    unit/test_optparse.c \
                    unit/test_config.c \
                    unit/test_stomp.c \
                    unit/test_signal.c \
                    unit/test_daemon.c \
                    unit/test_queue.c \
                    unit/test_transaction.c \
                    proto/test_connect.c \
                    proto/test_disconnect.c \
                    proto/test_begin.c \
                    proto/test_subscribe.c \
                    proto/test_error.c
newt_test_LDADD = -lcunit -l_newt -lconfuse -lpthread
newt_test_CFLAGS = -I../src/include/
newt_test_LDFLAGS = -L../src/lib/

check:
	./newt_test
