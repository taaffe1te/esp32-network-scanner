#pragma once

#include <Arduino.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/ip_addr.h>
#include <lwip/etharp.h>
#include <lwip/netif.h>


struct IcmpHeader {
  uint8_t  type;
  uint8_t  code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;       
};


struct PingResult {
  bool    alive;
  uint8_t mac[6];
  bool    hasMac;
};


class IcmpPing {
public:


  PingResult ping(uint32_t ipHostOrder, uint16_t timeoutMs = 150) {
    PingResult res = { false, {0,0,0,0,0,0}, false };


    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) return res;

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = (uint32_t)timeoutMs * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family      = AF_INET;
    dest.sin_addr.s_addr = htonl(ipHostOrder);


    uint8_t packet[sizeof(IcmpHeader)];
    IcmpHeader *hdr = (IcmpHeader *)packet;
    hdr->type     = 8;
    hdr->code     = 0;
    hdr->checksum = 0;
    hdr->id       = htons(0xBEEF);
    hdr->seq      = htons(_seq++);
    hdr->checksum = calcChecksum(packet, sizeof(packet));


    int sent = sendto(sock, packet, sizeof(packet), 0,
                      (struct sockaddr*)&dest, sizeof(dest));
    if (sent < 0) { close(sock); return res; }


    uint8_t  buf[64];
    struct   sockaddr_in from;
    socklen_t fromLen = sizeof(from);

    int received = recvfrom(sock, buf, sizeof(buf), 0,
                            (struct sockaddr*)&from, &fromLen);
    close(sock);

    if (received < 28) return res;


    IcmpHeader *reply = (IcmpHeader *)(buf + 20);
    if (reply->type != 0) return res; 

    res.alive = true;


    delay(10);


    res.hasMac = getMacFromArp(ipHostOrder, res.mac);

    return res;
  }

private:

  uint16_t _seq = 1;


  uint16_t calcChecksum(const uint8_t *buf, size_t len) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)buf;
    while (len > 1) {
      sum += *ptr++;
      len -= 2;
    }
    if (len == 1) sum += *(uint8_t *)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
  }


  bool getMacFromArp(uint32_t ipHostOrder, uint8_t mac[6]) {
    ip4_addr_t target;
    IP4_ADDR(&target,
      (ipHostOrder >> 24) & 0xFF,
      (ipHostOrder >> 16) & 0xFF,
      (ipHostOrder >>  8) & 0xFF,
       ipHostOrder        & 0xFF);

    struct netif     *ni      = netif_default;
    struct eth_addr  *ethAddr = nullptr;
    ip4_addr_t *found = nullptr;

    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
      if (etharp_get_entry(i, &found, &ni, &ethAddr)) {
        if (found && ip4_addr_eq(found, &target) && ethAddr) {
          memcpy(mac, ethAddr->addr, 6);
          return true;
        }
      }
    }
    return false;
  }
};
