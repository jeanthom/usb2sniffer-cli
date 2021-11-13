#ifndef PCAPEXPORT_H
#define PCAPEXPORT_H

#include <stdint.h>
#include "packing.h"

extern "C" {
	PACK(typedef struct pcap_hdr_s {
    	uint32_t magic_number; /* magic number */
    	uint16_t version_major; /* major version number */
    	uint16_t version_minor; /* minor version number */
    	int32_t  thiszone; /* GMT to local correction */
    	uint32_t sigfigs; /* accuracy of timestamps */
    	uint32_t snaplen; /* max length of captured packets, in octets */
    	uint32_t network; /* data link type */
	}) pcap_hdr_t;

	PACK(typedef struct pcaprec_hdr_s {
    	uint32_t ts_sec; /* timestamp seconds */
    	uint32_t ts_usec; /* timestamp microseconds */
    	uint32_t incl_len; /* number of octets of packet saved in file */
    	uint32_t orig_len; /* actual length of packet */
	}) pcaprec_hdr_t;
}

class PcapExport {
	FILE *fh;
	void writeGlobalHeader(void);

public:
	PcapExport(const char* filename);
	void writePacket(uint64_t ts, size_t len, void *data);
	~PcapExport(void);
	void write(void);
};

#endif /* PCAPEXPORT_H */