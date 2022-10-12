CC = gcc
CFLAGS = -Wall -g -O2

all: vak_client

vak_client: vak_client.c overlap_algo.c vrt.c tweetnacl.c

test: vak_client
	valgrind -s --leak-check=yes ./vak_client


