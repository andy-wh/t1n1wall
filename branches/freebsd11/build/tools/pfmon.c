/* pfmon , like ipmon for ipf, just for pf                               */
/* based on Simple Raw Sniffer by Luis Martin Garcia.                   */ 
/* and tcpdump print-pflog                                               */

#include <pcap.h> 
#include <string.h> 
#include <stdlib.h> 
#include <net/if.h>
#include <net/if_pflog.h> 
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <syslog.h>

#define MAXBYTES2CAPTURE 2048 

struct ip *iphdr = NULL;
struct tcphdr *tcphdr = NULL;
struct udphdr *udphdr = NULL;

typedef struct pfloghdr pfloghdr_t;

static char pfReason[][16] = 
{
    "match",
    "bad-offset",
    "fragment",
    "short",
    "normalize",
    "memory",
    "bad-timestamp",
    "congestion",
    "ip-option",
    "proto-cksum",
    "state-mismatch",
    "state-insert",
    "state-limit",
    "src-limit",
    "synproxy"
};

static char pfAction[][16]=
{
    "pass",
    "block",
    "scrub",
    "nat",
    "no-nat",
    "binat",
    "no-binat",
    "rdr",
    "no-rdr",
    "synproxy-drop"
};

static char pfDirection[][8] =
{
     "in/out",
     "in",
     "out"
};

static uint pfIPver[] = {0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0};

void processPacket(u_char *arg, const struct pcap_pkthdr* pkthdr, const u_char * packet){ 

    pfloghdr_t *pflogheader = NULL;
    pflogheader = (struct pfloghdr *) (packet);

    iphdr = (struct ip *)(packet + PFLOG_HDRLEN);
    int iphdrlen = 4*iphdr->ip_hl;
    int ippktlen =  ntohs(iphdr->ip_len);
    char pktdetails[256]; 

    char ipsrc[INET_ADDRSTRLEN];
    char ipdst[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(iphdr->ip_src), ipsrc, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(iphdr->ip_dst), ipdst, INET_ADDRSTRLEN);

    char srcip = inet_ntoa(iphdr->ip_src);
    char dstip = inet_ntoa(iphdr->ip_dst);
 
    sprintf(pktdetails, "%s -> %s PR %u len ", 
        inet_ntoa(iphdr->ip_src),inet_ntoa(iphdr->ip_dst), iphdr->ip_p,
        iphdrlen,  ippktlen - iphdrlen
        );

    if (iphdr->ip_p == IPPROTO_TCP) {
        tcphdr = (struct tcphdr *) ((unsigned int)iphdr+4*iphdr->ip_hl);
        sprintf(pktdetails, "%s,%u -> %s,%u PR tcp len %u %u", 
        	ipsrc, ntohs(tcphdr->th_sport),
        	ipdst, ntohs(tcphdr->th_dport),
         	iphdrlen,  ippktlen - iphdrlen
        	);
        if (tcphdr->th_flags) {
            sprintf(pktdetails + strlen(pktdetails)," -");
            if (tcphdr->th_flags & TH_FIN ) sprintf(pktdetails + strlen(pktdetails),"F");
            if (tcphdr->th_flags & TH_SYN ) sprintf(pktdetails + strlen(pktdetails),"S");
            if (tcphdr->th_flags & TH_RST ) sprintf(pktdetails + strlen(pktdetails),"R");
            if (tcphdr->th_flags & TH_PUSH) sprintf(pktdetails + strlen(pktdetails),"P");
            if (tcphdr->th_flags & TH_ACK ) sprintf(pktdetails + strlen(pktdetails),"A");
            if (tcphdr->th_flags & TH_URG ) sprintf(pktdetails + strlen(pktdetails),"U");
            if (tcphdr->th_flags & TH_ECE ) sprintf(pktdetails + strlen(pktdetails),"E");
            if (tcphdr->th_flags & TH_CWR ) sprintf(pktdetails + strlen(pktdetails),"C");
        }
    }
    if (iphdr->ip_p == IPPROTO_UDP) {
        udphdr =  (struct udphdr *)((unsigned int)iphdr+4*iphdr->ip_hl);
        sprintf(pktdetails, "%s,%u -> %s,%u PR udp len %u %u",
            ipsrc, ntohs(udphdr->uh_sport),
            ipdst, ntohs(udphdr->uh_dport),
        	iphdrlen,  ippktlen - iphdrlen
            );
    }
    if (iphdr->ip_p == IPPROTO_ICMP) {
    	sprintf(pktdetails, "%s ->  %s PR icmp len %u %u",
            ipsrc,ipdst,
            iphdrlen,  ippktlen - iphdrlen
            );
    }

    if (iphdr->ip_p == IPPROTO_IGMP) {
    	sprintf(pktdetails, "%s -> %s PR igmp len %u %u",
            inet_ntoa(iphdr->ip_src),inet_ntoa(iphdr->ip_dst),
            iphdrlen,  ippktlen - iphdrlen
            );
    }

    int i=0, *counter = (int *)arg; 

	uint32_t rulenr, subrulenr;
	char rulestr[32];
	rulenr = ntohl(pflogheader->rulenr);
	subrulenr = ntohl(pflogheader->subrulenr);

    if (pflogheader->subrulenr == (uint32_t)-1) {
    	sprintf(rulestr, "%u", rulenr);
    } else {
    	sprintf(rulestr, "%u.%s.%u %u",rulenr, pflogheader->ruleset, subrulenr, (uint32_t)1);
    }
    // get time from pcap pkthdr->ts
    struct tm *pkttm;
    char tmbuf[64];
    pkttm = localtime(&pkthdr->ts.tv_sec);
    strftime(tmbuf, sizeof tmbuf, "%H:%M:%S", pkttm);

    // log to syslog
      openlog ("pfmon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
      syslog (LOG_INFO, "%s.%06ld %s @%s %s %s %s %s",
    	tmbuf, (long int)(pkthdr->ts.tv_usec),
    	pflogheader->ifname, rulestr, pfAction[pflogheader->action], 
    	pktdetails,
    	pfDirection[pflogheader->dir], pfReason[pflogheader->reason]);
      closelog ();

     return; 
} 

/* main(): Main function. Opens network interface and calls pcap_loop() */
int main(int argc, char *argv[] ){ 

    /* go into background */
    if (daemon(0, 0) == -1)
    exit(1);
    /* write PID to file */
    FILE *pidfd;
    pidfd = fopen(argv[2], "w");
    if (pidfd)
    {
        fprintf(pidfd, "%d", getpid());
        fclose(pidfd);
    }

    int i=0, count=0; 
    pcap_t *descr = NULL; 
    char errbuf[PCAP_ERRBUF_SIZE], *device=NULL; 
    memset(errbuf,0,PCAP_ERRBUF_SIZE); 

     if( argc > 1){  /* If user supplied interface name, use it. */
        device = argv[1];
     } else {  /* Get the name of the first device suitable for capture */ 

        if ( (device = pcap_lookupdev(errbuf)) == NULL){
            fprintf(stderr, "ERROR: %s\n", errbuf);
            exit(1);
        }
     }

     printf("Opening device %s\n", device); 
     
     /* Open device in promiscuous mode */ 
     if ( (descr = pcap_open_live(device, MAXBYTES2CAPTURE, 1,  512, errbuf)) == NULL){
        fprintf(stderr, "ERROR: %s\n", errbuf);
        exit(1);
     }

     /* Loop forever & call processPacket() for every received packet*/ 
     if ( pcap_loop(descr, -1, processPacket, (u_char *)&count) == -1){
        fprintf(stderr, "ERROR: %s\n", pcap_geterr(descr) );
        exit(1);
     }

return 0; 

} 

