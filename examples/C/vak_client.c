#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "vrt.h"
#include "overlap_algo.h"

/* Number of overlapping responses required to succeed */
static const int WANTED_OVERLAPS = 10;

/* Maximum uncertainty (seconds) of overlap required to succeed */
static const double WANTED_UNCERTAINTY = 2.0;

/* How long to wait for a successful response to a roughtime query */
static const uint64_t QUERY_TIMEOUT_USECS = 1000000;

/** Structure describing a roughtime server */
struct rt_server {
    /** Hostname or IP address */
    const char *host;

    /** Port number */
    unsigned port;

    /** Protocol variant, the roughtime draft number */
    int variant;

    /** Server's ed2551 public key */
    uint8_t public_key[32];
};

/* List of roughtime servers from
 * https://github.com/cloudflare/roughtime/blob/master/ecosystem.json
 * with additional Netnod's test servers.
 *
 * Note tht falseticker.roughtime.netnod.se are for testing and will
 * usually return bad time.  This is intended for testing of the
 * algorithms used in "vad är klockan".
 *
 * Note that the first set of servers is repeated a couple times so
 * that they will be a majority compared to the falsetickers at the
 * end.
 */

static struct rt_server server_list[] = {
    { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 5, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 5, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 5, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 5, { 0xb4,0x03,0xec,0x41,0xcd,0xc3,0xdf,0xa9,0x89,0x3c,0xe5,0xf5,0xfc,0xb2,0xcd,0x6d,0x5d,0x0c,0xdd,0xfb,0x93,0x3e,0x3c,0x16,0xe7,0x89,0x86,0xbf,0x0f,0x95,0xd6,0x11 } }, /* lab.roughtime.netnod.se */

    { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 5, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 5, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 5, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 5, { 0xb4,0x03,0xec,0x41,0xcd,0xc3,0xdf,0xa9,0x89,0x3c,0xe5,0xf5,0xfc,0xb2,0xcd,0x6d,0x5d,0x0c,0xdd,0xfb,0x93,0x3e,0x3c,0x16,0xe7,0x89,0x86,0xbf,0x0f,0x95,0xd6,0x11 } }, /* lab.roughtime.netnod.se */

    { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 5, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 5, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 5, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 5, { 0xb4,0x03,0xec,0x41,0xcd,0xc3,0xdf,0xa9,0x89,0x3c,0xe5,0xf5,0xfc,0xb2,0xcd,0x6d,0x5d,0x0c,0xdd,0xfb,0x93,0x3e,0x3c,0x16,0xe7,0x89,0x86,0xbf,0x0f,0x95,0xd6,0x11 } }, /* lab.roughtime.netnod.se */

    { "209.50.50.228", 2002, 4, { 0x88,0x15,0x63,0xc6,0x0f,0xf5,0x8f,0xbc,0xb5,0xfa,0x44,0x14,0x4c,0x16,0x1d,0x4d,0xa6,0xf1,0x0a,0x9a,0x5e,0xb1,0x4f,0xf4,0xec,0x3e,0x0f,0x30,0x32,0x64,0xd9,0x60 } }, /* caesium.tannerryan.ca */
    { "162.159.200.123", 2002, 4, { 0x80,0x3e,0xb7,0x85,0x28,0xf7,0x49,0xc4,0xbe,0xc2,0xe3,0x9e,0x1a,0xbb,0x9b,0x5e,0x5a,0xb7,0xe4,0xdd,0x5c,0xe4,0xb6,0xf2,0xfd,0x2f,0x93,0xec,0xc3,0x53,0x8f,0x1a } }, /* roughtime.cloudflare.com */
    { "35.192.98.51", 2002, 4, { 0x01,0x6e,0x6e,0x02,0x84,0xd2,0x4c,0x37,0xc6,0xe4,0xd7,0xd8,0xd5,0xb4,0xe1,0xd3,0xc1,0x94,0x9c,0xea,0xa5,0x45,0xbf,0x87,0x56,0x16,0xc9,0xdc,0xe0,0xc9,0xbe,0xc1 } }, /* roughtime.int08h.com */
    { "192.36.143.134", 2002, 5, { 0x4b,0x70,0x33,0x7d,0x92,0x79,0x0a,0x34,0x9d,0x90,0x9d,0xb5,0x64,0x91,0x9b,0xc6,0xa7,0x58,0x3f,0xf4,0xa8,0x13,0xc7,0xd7,0x29,0x8d,0x3e,0x6a,0x27,0x2c,0x7a,0x12 } }, /* roughtime.se */
    { "194.58.207.198", 2002, 5, { 0xf6,0x5d,0x49,0x37,0x81,0xda,0x90,0x69,0xc6,0xe3,0x8c,0xb2,0xab,0x23,0x4d,0x09,0xbd,0x07,0x37,0x45,0xdf,0xb3,0x2b,0x01,0x6e,0x79,0x7f,0x91,0xb6,0x68,0x64,0x37 } }, /* sth1.roughtime.netnod.se */
    { "194.58.207.199", 2002, 5, { 0x4f,0xfc,0x71,0x5f,0x81,0x11,0x50,0x10,0x0e,0xa6,0xde,0xb8,0x67,0xca,0x61,0x59,0xa9,0x8a,0xb0,0x04,0x99,0xc4,0x9d,0x15,0x5a,0xe8,0x8f,0x9b,0x71,0x92,0xff,0xc8 } }, /* sth2.roughtime.netnod.se */
    { "194.58.207.196", 2002, 5, { 0xb4,0x03,0xec,0x41,0xcd,0xc3,0xdf,0xa9,0x89,0x3c,0xe5,0xf5,0xfc,0xb2,0xcd,0x6d,0x5d,0x0c,0xdd,0xfb,0x93,0x3e,0x3c,0x16,0xe7,0x89,0x86,0xbf,0x0f,0x95,0xd6,0x11 } }, /* lab.roughtime.netnod.se */

    { "194.58.207.197", 2002, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2000, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2001, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2003, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2004, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2005, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2006, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2007, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2008, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
    { "194.58.207.197", 2009, 5, { 0x88,0x56,0x3d,0x82,0x52,0x27,0xf1,0x21,0xc6,0xb6,0x41,0x53,0x75,0x41,0x02,0x61,0xd0,0xb7,0xed,0x3e,0x0f,0x34,0xcd,0x98,0x48,0x5c,0xe3,0x6c,0x46,0xe6,0x7d,0x92 } }, /* falseticker.roughtime.netnod.se */
};

/** Get the wall time.
 *
 * This functions uses the same representation of time as the vrt
 * functions, i.e. the number of microseconds the epoch which is
 * midnight 1970-01-01.
 *
 * \returns The number of microseconds since the epoch.
 */
static uint64_t get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t t =
        (uint64_t)(tv.tv_sec) * 1000000 +
        (uint64_t)(tv.tv_usec);
    return t;
}

/* Query a roughtime server to find out how to adjust our local clock
 * to match the time from the server.
 *
 * The adjustment is a range (lo, hi) which takes into account the
 * server's reported uncertainty and the round trip time for the
 * query.
 *
 * \param server a rt_server structure describing the server to query
 * \param lo the low value of the range is written to this pointer
 * \param hi the high value of the range is written to this pointer
 * \returns 1 on success or 0 on failure
 *
 * This function contains some platform specific code.
 * Implementations for other platforms will look slightly different.
 * The concepts should be the same though.
 *
 * Set up an UDP socket.
 *
 * Create a random nonce.
 *
 * Fill in a buffer with a roughtime query by calling vrt_make_query.
 *
 * Save the time the query was sent.
 *
 * Send the query to the server and wait for a response.
 *
 * Save the time the response was recevied.
 *
 * Process the response using vrt_parse_response which also verifies
 * the signature and that the nonce matches.  If that succeeded,
 * return the lo and hi adjustments.
 *
 * If the processing failed, and the request has not timed out yet, go
 * back and wait for more responses.
 */
static int query_server(struct rt_server *server, overlap_value_t *lo, overlap_value_t *hi)
{
    uint32_t recv_buffer[VRT_QUERY_PACKET_LEN / 4] = {0};
    uint8_t query_buf[VRT_QUERY_PACKET_LEN] = {0};
    uint32_t query_buf_len;
    struct sockaddr_in servaddr;
    struct sockaddr_in respaddr;
    int sockfd;
    uint64_t st, rt;
    struct hostent *he;
    uint8_t nonce[VRT_NONCE_SIZE];
    uint64_t out_midpoint;
    uint32_t out_radii;

    /* Create a random nonce.  This should be as good randomness as
     * possible, preferably cryptographically secure randomness. */
    if (getentropy(nonce, sizeof(nonce)) < 0) {
        fprintf(stderr, "getentropy(%u) failed: %s\n", (unsigned)sizeof(nonce), strerror(errno));
        return 0;
    }

    /* Fill in the query. */
    query_buf_len = sizeof(query_buf);
    if (vrt_make_query(nonce, 64, query_buf, &query_buf_len, server->variant) != VRT_SUCCESS) {
        fprintf(stderr, "vrt_make_query failed\n");
        return 0;
    }

    printf("%s:%u: variant %d size %u ", server->host, server->port, server->variant, (unsigned)query_buf_len);
    fflush(stdout);

    /* Look up the host name or process the IPv4 address and fill in
     * the servaddr structure. */
    he = gethostbyname(server->host);
    if (!he) {
        fprintf(stderr, "gethostbyname failed: %s\n", strerror(errno));
        return 0;
    }

    memset((char *)&servaddr, 0, sizeof(servaddr));
    memcpy(&servaddr.sin_addr.s_addr, he->h_addr_list[0], sizeof(struct in_addr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server->port);

    /* Create an UDP socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        return 0;
    }

    /* Get the wall time just before the request was sent out. */
    st = get_time();

    /* Send the request */
    int n = sendto(sockfd, (const char *)query_buf, query_buf_len, 0,
                   (const struct sockaddr *)&servaddr, sizeof(servaddr));
    if (n != query_buf_len) {
        fprintf(stderr, "sendto failed: %s\n", strerror(errno));
        close(sockfd);
        return 0;
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    /* Keep waiting until we get a valid response or we time out */
    while (1) {
        fd_set readfds;
        int n;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        /* Use a tenth of the query timeout for the select call. */
        tv.tv_sec = 0;
        tv.tv_usec = QUERY_TIMEOUT_USECS / 10;

        n = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "select failed: %s\n", strerror(errno));
            close(sockfd);
            return 0;
        }

        /* Get the time as soon after the packet was received. */
        rt = get_time();

        /* Get the packet from the kernel. */
        if (FD_ISSET(sockfd, &readfds)) {
            socklen_t respaddrsize = sizeof(respaddr);
            n = recvfrom(sockfd, recv_buffer,
                         (sizeof recv_buffer) * sizeof recv_buffer[0],
                         0 /* flags */,
                         (struct sockaddr *)&respaddr, &respaddrsize);
            if (n < 0) {
                if (errno == EAGAIN)
                    continue;
                fprintf(stderr, "recv failed: %s\n", strerror(errno));
                close(sockfd);
                return 0;
            }

            /* TODO We might want to verify that servaddr and respaddr
             * match before parsing the response.  This way we could
             * discard faked packets without having to verify the
             * signature.  */

            /* Verify the response, check the signature and that it
             * matches the nonce we put in the query. */
            if (vrt_parse_response(nonce, 64, recv_buffer,
                                   n,
                                   server->public_key, &out_midpoint,
                                   &out_radii, server->variant) != VRT_SUCCESS) {
                fprintf(stderr, "vrt_parse_response failed\n");
                continue;
            }

            /* Break out of the loop. */
            break;
        }

        if (rt - st > QUERY_TIMEOUT_USECS) {
            printf("timeout\n");
            close(sockfd);
            return 0;
        }
    }

    close(sockfd);

    /* Translate roughtime response to lo..hi adjustment range.  */
    uint64_t local_time = (st + rt) / 2;
    double adjustment = adjustment = ((double)out_midpoint - (double)local_time)/1000000;
    double rtt = (double)(rt - st) / 1000000;
    double uncertainty = (double)out_radii / 1000000 + rtt / 2;

    *lo = adjustment - uncertainty;
    *hi = adjustment + uncertainty;

    printf("adj %.6f .. %.6f\n", *lo, *hi);

    return 1;
}

int main(int argc, char *argv[]) {
    struct rt_server *randomized_servers[sizeof(server_list) / sizeof(server_list[0])];
    struct overlap_algo *algo;
    int success = 0;
    int nr_responses = 0;
    int nr_overlaps;
    overlap_value_t lo, hi;
    int i;
    unsigned seed;

    // TODO add command line argument "-q" to only query
    // TODO add command line argument "-v" for verbose

    /* Seed the glibc random function with some randomness.  This is
     * used to randomize the list of servers. */
    if (getentropy(&seed, sizeof(seed)) < 0) {
        fprintf(stderr, "getentropy(%u) failed: %s\n", (unsigned)sizeof(seed), strerror(errno));
        exit(1);
    }
    srandom(seed);

    /* Create a list of servers with the order randomized */
    for (i = 0; i < sizeof(server_list) / sizeof(server_list[0]); i++)
        randomized_servers[i] = &server_list[i];
    for (i = sizeof(server_list) / sizeof(server_list[0]); i > 0; i--) {
        struct rt_server *t;
        int j = random() % i;
        printf("%d ", j);
        t = randomized_servers[i-1];
        randomized_servers[i-i] = randomized_servers[j];
        randomized_servers[j] = t;
    }
    printf("\n");

    /* Create the overlap algorithm */
    algo = overlap_new();
    if (!algo) {
        fprintf(stderr, "overlap_new failed\n");
        exit(1);
    }

    /* Query the randomized list of servers until we have a majority
     * of responses which all overlap, the number of overlaps is
     * enough and the uncertainty is low enough */
    for (i = 0; i < sizeof(randomized_servers) / sizeof(randomized_servers[0]); i++) {
        if (!query_server(randomized_servers[i], &lo, &hi))
            continue;

        nr_responses++;

        overlap_add(algo, lo, hi);
        nr_overlaps = overlap_find(algo, &lo, &hi);

        if (nr_overlaps > nr_responses / 2 && nr_overlaps >= WANTED_OVERLAPS && (hi - lo) <= WANTED_UNCERTAINTY) {
            success = 1;
            break;
        }
    }

    /* Delete the overlap algorithm since we're finished with it. */
    overlap_del(algo);

    /* Exit with an error if we didn't succeed */
    if (!success) {
        printf("failure: unable to get %d overlapping responses\n", WANTED_OVERLAPS);
        exit(1);
    }

    printf("success: %d/%d overlapping responses, range %.6f .. %.6f\n", nr_overlaps, nr_responses, lo, hi);

    /* If this code is enabled, adjust the local clock. */
    if (0) {
        struct timeval tv;
        /* Take the middle of the adjustment range */
        double adjustment = (lo + hi) / 2;
        gettimeofday(&tv, NULL);
        tv.tv_sec += adjustment; /* integer portion of adjustment */
        tv.tv_usec += (adjustment - tv.tv_sec) * 1000000; /* fractional part converted to microseconds */
        settimeofday(&tv, NULL);
    }

    exit(0);
}
