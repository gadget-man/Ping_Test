/*
 * Copyright (c) 2018-2020 iParcelBox Limited
 * All rights reserved
 *
 */

#include "string.h"
#include "ping.h"
#include "ping_sock.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "common/cs_dbg.h" //PMN

esp_ping_handle_t ping;


static void test_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    // optionally, get callback arguments
    // const char* str = (const char*) args;
    // printf("%s\r\n", str); // "foo"
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    LOG(LL_INFO,("PING %d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n",
            recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time));
}

static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    LOG(LL_INFO,("PING From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), seqno));
}

static void test_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    LOG(LL_INFO,("PING %d packets transmitted, %d received, time %dms\n", transmitted, received, total_time_ms));
}

void initialize_ping()
{
    /* convert URL to IP address */
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));
    int result = getaddrinfo("www.espressif.com", NULL, &hint, &res);
    if((result != 0) || (res == NULL))
    {
        LOG(LL_INFO, ("DNS lookup failed err=%d res=%p", result, res));
    } else {
        LOG(LL_INFO, ("Initialising ping"));
        struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
        freeaddrinfo(res);

        esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
        ping_config.target_addr = target_addr;          // target IP address
        ping_config.count = 1;    // ping in infinite mode, esp_ping_stop can stop it

        /* set callback functions */
        esp_ping_callbacks_t cbs;
        cbs.on_ping_success = test_on_ping_success;
        cbs.on_ping_timeout = test_on_ping_timeout;
        cbs.on_ping_end = test_on_ping_end;
        cbs.cb_args = NULL;
        // cbs.cb_args = "foo";  // arguments that will feed to all callback functions, can be NULL
        // cbs.cb_args = eth_event_group;

        esp_ping_new_session(&ping_config, &cbs, &ping);
        esp_ping_start(ping);
    }
}

bool enable_ping(void) {
    initialize_ping();
    esp_ping_start(ping);
    return true;
}
