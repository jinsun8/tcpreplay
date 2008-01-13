/*
 * pkt.c
 *
 * Copyright (c) 2001 Dug Song <dugsong@monkey.org>
 *
 * $Id: pkt.c,v 1.10 2002/04/07 22:55:20 dugsong Exp $
 */

#include "config.h"

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bget.h"
#include "pkt.h"

void
pkt_init(int size)
{
	bectl(NULL, malloc, free, sizeof(struct pkt) * size);
}

struct pkt *
pkt_new(void)
{
	struct pkt *pkt;
	
	if ((pkt = bget(sizeof(*pkt))) == NULL)
		return (NULL);
	
	timerclear(&pkt->pkt_ts);
    // memset(&pkt->pkt_ev, 0, sizeof(pkt->pkt_ev));

	pkt->pkt_data = pkt->pkt_buf + PKT_BUF_ALIGN;
	pkt->pkt_eth = (struct eth_hdr *)pkt->pkt_data;
	pkt->pkt_eth_data = pkt->pkt_data + ETH_HDR_LEN;
	pkt->pkt_ip_data = pkt->pkt_data + ETH_HDR_LEN + IP_HDR_LEN;
	pkt->pkt_tcp_data = NULL;
	pkt->pkt_end = pkt->pkt_ip_data;
	
	return (pkt);
}

struct pkt *
pkt_dup(struct pkt *pkt)
{
	struct pkt *new;
	off_t off;
	
	if ((new = bget(sizeof(*new))) == NULL)
		return (NULL);
	
	off = new->pkt_buf - pkt->pkt_buf;
	
	new->pkt_ts = pkt->pkt_ts;
    // memset(&new->pkt_ev, 0, sizeof(new->pkt_ev));

	new->pkt_data = pkt->pkt_data + off;
	
	new->pkt_eth = (pkt->pkt_eth != NULL) ?
	    (struct eth_hdr *)new->pkt_data : NULL;
	
	new->pkt_eth_data = (pkt->pkt_eth_data != NULL) ?
	    pkt->pkt_eth_data + off : NULL;
	
	new->pkt_ip_data = (pkt->pkt_ip_data != NULL) ?
	    pkt->pkt_ip_data + off : NULL;
	
	new->pkt_tcp_data = (pkt->pkt_tcp_data != NULL) ?
	    pkt->pkt_tcp_data + off : NULL;
	
	memcpy(new->pkt_data, pkt->pkt_data, pkt->pkt_end - pkt->pkt_data);
	
	new->pkt_end = pkt->pkt_end + off;
	
	return (new);
}

void
pkt_decorate(struct pkt *pkt)
{
	u_char *p;
	int hl, len, off;

	pkt->pkt_data = pkt->pkt_buf + PKT_BUF_ALIGN;
	pkt->pkt_eth = NULL;
	pkt->pkt_ip = NULL;
	pkt->pkt_ip_data = NULL;
	pkt->pkt_tcp_data = NULL;

	p = pkt->pkt_data;
	
	if (p + ETH_HDR_LEN > pkt->pkt_end)
		return;

	pkt->pkt_eth = (struct eth_hdr *)p;
	p += ETH_HDR_LEN;

	if (p + IP_HDR_LEN > pkt->pkt_end)
		return;
	
	pkt->pkt_eth_data = p;
	
	/* If IP header length is longer than packet length, stop. */
	hl = pkt->pkt_ip->ip_hl << 2;
	if (p + hl > pkt->pkt_end) {
		pkt->pkt_ip = NULL;
		return;
	}
	/* If IP length is longer than packet length, stop. */
	len = ntohs(pkt->pkt_ip->ip_len);
	if (p + len > pkt->pkt_end)
		return;

	/* If IP fragment, stop. */
	off = ntohs(pkt->pkt_ip->ip_off);
	if ((off & IP_OFFMASK) != 0 || (off & IP_MF) != 0)
		return;
	
	pkt->pkt_end = p + len;
	p += hl;

	/* If transport layer header is longer than packet length, stop. */
	switch (pkt->pkt_ip->ip_p) {
	case IP_PROTO_ICMP:
		hl = ICMP_HDR_LEN;
		break;
	case IP_PROTO_TCP:
		if (p + TCP_HDR_LEN > pkt->pkt_end)
			return;
		hl = ((struct tcp_hdr *)p)->th_off << 2;
		break;
	case IP_PROTO_UDP:
		hl = UDP_HDR_LEN;
		break;
	default:
		return;
	}
	if (p + hl > pkt->pkt_end)
		return;
	
	pkt->pkt_ip_data = p;
	p += hl;

	/* Check for transport layer data. */
	switch (pkt->pkt_ip->ip_p) {
	case IP_PROTO_ICMP:
		pkt->pkt_icmp_msg = (union icmp_msg *)p;
		
		switch (pkt->pkt_icmp->icmp_type) {
		case ICMP_ECHO:
		case ICMP_ECHOREPLY:
			hl = sizeof(pkt->pkt_icmp_msg->echo);
			break;
		case ICMP_UNREACH:
			if (pkt->pkt_icmp->icmp_code == ICMP_UNREACH_NEEDFRAG)
				hl = sizeof(pkt->pkt_icmp_msg->needfrag);
			else
				hl = sizeof(pkt->pkt_icmp_msg->unreach);
			break;
		case ICMP_SRCQUENCH:
		case ICMP_REDIRECT:
		case ICMP_TIMEXCEED:
		case ICMP_PARAMPROB:
			hl = sizeof(pkt->pkt_icmp_msg->srcquench);
			break;
		case ICMP_RTRADVERT:
			hl = sizeof(pkt->pkt_icmp_msg->rtradvert);
			break;
		case ICMP_RTRSOLICIT:
			hl = sizeof(pkt->pkt_icmp_msg->rtrsolicit);
			break;
		case ICMP_TSTAMP:
		case ICMP_TSTAMPREPLY:
			hl = sizeof(pkt->pkt_icmp_msg->tstamp);
			break;
		case ICMP_INFO:
		case ICMP_INFOREPLY:
		case ICMP_DNS:
			hl = sizeof(pkt->pkt_icmp_msg->info);
			break;
		case ICMP_MASK:
		case ICMP_MASKREPLY:
			hl = sizeof(pkt->pkt_icmp_msg->mask);
			break;
		case ICMP_DNSREPLY:
			hl = sizeof(pkt->pkt_icmp_msg->dnsreply);
			break;
		default:
			hl = pkt->pkt_end - p + 1;
			break;
		}
		if (p + hl > pkt->pkt_end)
			pkt->pkt_icmp_msg = NULL;
		break;
	case IP_PROTO_TCP:
		if (p < pkt->pkt_end)
			pkt->pkt_tcp_data = p;
		break;
	case IP_PROTO_UDP:
		if (pkt->pkt_ip_data + ntohs(pkt->pkt_udp->uh_ulen) <=
		    pkt->pkt_end)
			pkt->pkt_udp_data = p;
		break;
	}
}

void
pkt_free(struct pkt *pkt)
{
	brel(pkt);
}

void
pktq_reverse(struct pktq *pktq)
{
	struct pktq tmpq;
	struct pkt *pkt, *next;

	TAILQ_INIT(&tmpq);
	for (pkt = TAILQ_FIRST(pktq); pkt != TAILQ_END(pktq); pkt = next) {
		next = TAILQ_NEXT(pkt, pkt_next);
		TAILQ_INSERT_HEAD(&tmpq, pkt, pkt_next);
	}
	*pktq = tmpq;
}

void
pktq_shuffle(rand_t *r, struct pktq *pktq)
{
	static struct pkt **pvbase;
	static int pvlen;
	struct pkt *pkt;
	int i;

	i = 0;
	TAILQ_FOREACH(pkt, pktq, pkt_next) {
		i++;
	}
	if (i > pvlen) {
		pvlen = i;
		if (pvbase == NULL)
			pvbase = malloc(sizeof(pkt) * pvlen);
		else
			pvbase = realloc(pvbase, sizeof(pkt) * pvlen);
	}
	i = 0;
	TAILQ_FOREACH(pkt, pktq, pkt_next) {
		pvbase[i++] = pkt;
	}
	TAILQ_INIT(pktq);
	
	rand_shuffle(r, pvbase, i, sizeof(pkt));

	while (--i >= 0) {
		TAILQ_INSERT_TAIL(pktq, pvbase[i], pkt_next);
	}
}

struct pkt *
pktq_random(rand_t *r, struct pktq *pktq)
{
	struct pkt *pkt;
	int i;
	
	i = 0;
	TAILQ_FOREACH(pkt, pktq, pkt_next) {
		i++;
	}
	i = rand_uint32(r) % (i - 1);
	pkt = TAILQ_FIRST(pktq);
	
	while (--i >= 0) {
		pkt = TAILQ_NEXT(pkt, pkt_next);
	}
	return (pkt);
}
