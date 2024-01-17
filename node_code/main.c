#include <stdio.h>
#include <string.h>

#include "net/gcoap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "od.h"
#include "fmt.h"

#include "lpsxxx.h"
#include "lpsxxx_params.h"

#include "isl29020.h"
#include "isl29020_params.h"

#include "xtimer.h"

#include "thread.h"

#include "mutex.h"

static lpsxxx_t lpsxxx;
static isl29020_t isl29020;

static bool _proxied = false;
static sock_udp_ep_t _proxy_remote;

#define _LAST_REQ_PATH_MAX (64)
static char _last_req_path[_LAST_REQ_PATH_MAX];

/* Counts requests sent by CLI. */
static uint16_t req_count = 0;

float temp_mean = 39.456;
float temp_stddev = 789.2; 

float pressure_mean = 992;
float pressure_stddev = 99.2; 

mutex_t coap_mutex; 

#define SERVER_ADDR "2a05:d016:1e1:9194:9c47:cc16:d6ae:84a"
#define SERVER_PORT "5683"

char temp_thread_stack[THREAD_STACKSIZE_MAIN];
char pres_thread_stack[THREAD_STACKSIZE_MAIN];
char light_thread_stack[THREAD_STACKSIZE_MAIN];

static void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    (void)remote;       /* not interested in the source currently */

    if (memo->state == GCOAP_MEMO_TIMEOUT) {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }
    else if (memo->state == GCOAP_MEMO_ERR) {
        printf("gcoap: error in response\n");
        return;
    }

    coap_block1_t block;
    if (coap_get_block2(pdu, &block) && block.blknum == 0) {
        puts("--- blockwise start ---");
    }

    char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
                            ? "Success" : "Error";
    printf("gcoap: response %s, code %1u.%02u", class_str,
                                                coap_get_code_class(pdu),
                                                coap_get_code_detail(pdu));
    if (pdu->payload_len) {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT
                || content_type == COAP_FORMAT_LINK
                || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
                || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
            /* Expecting diagnostic payload in failure cases */
            printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
                                                          (char *)pdu->payload);
        }
        else {
            printf(", %u bytes\n", pdu->payload_len);
            od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        }
    }
    else {
        printf(", empty payload\n");
    }

    /* ask for next block if present */
    if (coap_get_block2(pdu, &block)) {
        if (block.more) {
            unsigned msg_type = coap_get_type(pdu);
            if (block.blknum == 0 && !strlen(_last_req_path)) {
                puts("Path too long; can't complete blockwise");
                return;
            }

            if (_proxied) {
                gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
                               COAP_METHOD_GET, NULL);
            }
            else {
                gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
                               COAP_METHOD_GET, _last_req_path);
            }

            if (msg_type == COAP_TYPE_ACK) {
                coap_hdr_set_type(pdu->hdr, COAP_TYPE_CON);
            }
            block.blknum++;
            coap_opt_add_block2_control(pdu, &block);

            if (_proxied) {
                coap_opt_add_proxy_uri(pdu, _last_req_path);
            }

            int len = coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
            gcoap_req_send((uint8_t *)pdu->hdr, len, remote,
                           _resp_handler, memo->context);
        }
        else {
            puts("--- blockwise complete ---");
        }
    }
}

static bool _parse_endpoint(sock_udp_ep_t *remote,
                            char *addr_str, char *port_str)
{
    ipv6_addr_t addr;
    remote->family = AF_INET6;

    /* parse for interface */
    char *iface = ipv6_addr_split_iface(addr_str);
    if (!iface) {
        if (gnrc_netif_numof() == 1) {
            /* assign the single interface found in gnrc_netif_numof() */
            remote->netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else {
            remote->netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else {
        int pid = atoi(iface);
        if (gnrc_netif_get_by_pid(pid) == NULL) {
            puts("gcoap_cli: interface not valid");
            return false;
        }
        remote->netif = pid;
    }
    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("gcoap_cli: unable to parse destination address");
        return false;
    }
    if ((remote->netif == SOCK_ADDR_ANY_NETIF) && ipv6_addr_is_link_local(&addr)) {
        puts("gcoap_cli: must specify interface for link local target");
        return false;
    }
    memcpy(&remote->addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

    /* parse port */
    remote->port = atoi(port_str);
    if (remote->port == 0) {
        puts("gcoap_cli: unable to parse destination port");
        return false;
    }

    return true;
}

static size_t _send(uint8_t *buf, size_t len, char *addr_str, char *port_str)
{
    size_t bytes_sent;
    sock_udp_ep_t *remote;
    sock_udp_ep_t new_remote;

    if (_proxied) {
        remote = &_proxy_remote;
    }
    else {
        if (!_parse_endpoint(&new_remote, addr_str, port_str)) {
            return 0;
        }
        remote = &new_remote;
    }

    bytes_sent = gcoap_req_send(buf, len, remote, _resp_handler, NULL);
    if (bytes_sent > 0) {
        req_count++;
    }
    return bytes_sent;
}

static void send_coap_post_request(char *server_addr, char *server_port, char *path, char *data)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;
    mutex_lock(&coap_mutex);

    /* Initialize CoAP request */
    gcoap_req_init(&pdu, buf, CONFIG_GCOAP_PDU_BUF_SIZE, COAP_METHOD_POST, path);
    coap_hdr_set_type(pdu.hdr, COAP_TYPE_CON);  /* Confirmable request */

    /* Add CoAP options and payload */
    coap_opt_add_format(&pdu, COAP_FORMAT_TEXT);
    len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
    memcpy(pdu.payload, data, strlen(data));
    len += strlen(data);

    /* Send CoAP request */
    if (!_send(buf, len, server_addr, server_port)) {
        printf("gcoap: msg send failed\n");
    }
    xtimer_sleep(2);
    mutex_unlock(&coap_mutex);

}

float generate_normal_random(float stddev) {
    float M_PI = 3.1415926535;

    // Box-Muller transform to generate random numbers with normal distribut$    float u1 = rand() / (float)RAND_MAX;
    float u2 = rand() / (float)RAND_MAX;
    float z = sqrt(-2 * log(u1)) * cos(2 * M_PI * u2);
    
    return stddev * z;
}

float add_noise(float stddev) {
    int num;
    float noise_val = 0;
    
    num = rand() % 100 + 1; // use rand() function to get the random number  
    if (num >= 50) {
        // Generate a random number with normal distribution based on a stdd$        noise_val = generate_normal_random(stddev);
    }
    return noise_val;
}

void *thread_light(void *arg) {
    (void)arg;
    isl29020_init(&isl29020, &isl29020_params[0]);
    while (1) {
        int lux = isl29020_read(&isl29020);
        char payload_data[64];
        sprintf(payload_data, "%d", lux);
        printf("Thread light: %d", lux);
        send_coap_post_request(SERVER_ADDR, SERVER_PORT, "/light", payload_data);
        xtimer_sleep(4);
    }

    return NULL;
}

void *thread_temp(void *arg) {
    (void)arg;
    lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);
    while (1) {
        int16_t temp = 0;
        lpsxxx_read_temp(&lpsxxx, &temp);
        char payload_data[64];
        temp = temp + add_noise(temp_stddev);
        sprintf(payload_data, "%i.%u", (temp / 100), (temp % 100));
        printf("Thread temperature: %i.%u\n", (temp / 100), (temp % 100));   
-        send_coap_post_request(SERVER_ADDR, SERVER_PORT, "/temperature", pa$        xtimer_sleep(4);
    }

    return NULL;
}
void *thread_pres(void *arg) {
    (void)arg;
    lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);
    while (1) {
        uint16_t pres = 0;
        lpsxxx_read_pres(&lpsxxx, &pres);
        char payload_data[64];
        pres = pres + add_noise(pressure_stddev);
        sprintf(payload_data, "%d", pres);
        printf("Thread pressure: %d\n", pres);
        send_coap_post_request(SERVER_ADDR, SERVER_PORT, "/pressure", payloa$        xtimer_sleep(4);
    }

    return NULL;
}

int main(void)
{
    mutex_init(&coap_mutex);
    thread_create(temp_thread_stack, sizeof(temp_thread_stack),
                THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                thread_temp, NULL, "temp_thread");
    thread_create(pres_thread_stack, sizeof(pres_thread_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                thread_pres, NULL, "pres_thread");

    thread_create(light_thread_stack, sizeof(light_thread_stack),
                THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                thread_light, NULL, "light_thread");
}
