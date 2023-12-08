#include <stdio.h>
#include <string.h>

#include "net/gcoap.h"
#include "net/utils.h"

#include "lpsxxx.h"
#include "lpsxxx_params.h"
#include "xtimer.h"


static lpsxxx_t lpsxxx;

static void response_handler(const gcoap_request_memo_t *memo, coap_pkt_t *pdu, const sock_udp_ep_t *remote)
{
    (void)remote;  /* not interested in the source currently */

    if (memo->state == GCOAP_MEMO_TIMEOUT) {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        return;
    }

    /* Process the response as needed */
    printf("Response received: %.*s\n", (int)pdu->payload_len, (char *)pdu->payload);
}

static bool _parse_endpoint(sock_udp_ep_t *remote,
                            const char *addr_str, const char *port_str)
{
    netif_t *netif;

    /* parse hostname */
    if (netutils_get_ipv6((ipv6_addr_t *)&remote->addr, &netif, addr_str) < 0) {
        puts("gcoap_cli: unable to parse destination address");
        return false;
    }
    remote->netif = netif ? netif_get_id(netif) : SOCK_ADDR_ANY_NETIF;
    remote->family = AF_INET6;

    /* parse port */
    remote->port = atoi(port_str);
    if (remote->port == 0) {
        puts("gcoap_cli: unable to parse destination port");
        return false;
    }

    return true;
}

static size_t _send(uint8_t *buf, size_t len, const char *addr_str, const char *port_str)
{
    size_t bytes_sent;
    sock_udp_ep_t remote;

    if (!_parse_endpoint(&remote, addr_str, port_str)) {
        puts("gcoap_cli: unable to parse destination address");
        return 0;
    }

    bytes_sent = gcoap_req_send(buf, len, &remote, response_handler, NULL);
    return bytes_sent;
}

static void send_coap_post_request(const char *server_addr, const char *server_port, const char *path, const char *data)
{
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;

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
}

int main(void)
{
    const char *server_address = " 2a05:d016:1e1:9194:9c47:cc16:d6ae:84a";  // Replace with your CoAP server address
    const char *server_port = "5683";  // Replace with your CoAP server port
    const char *resource_path = "/temperature";  // Replace with your desired resource path
    char payload_data[64];
    lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);
        while (1) {
            uint16_t pres = 0;
            int16_t temp = 0;
            lpsxxx_read_temp(&lpsxxx, &temp);
            lpsxxx_read_pres(&lpsxxx, &pres);
            sprintf(payload_data,"P: %uhPa, T: %i.%u°C", pres, (temp / 100), (temp % 100));
        send_coap_post_request(server_address, server_port, resource_path, payload_data);
            xtimer_sleep(2);
}
    return 0;
}
