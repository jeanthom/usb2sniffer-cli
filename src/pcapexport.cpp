#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "pcapexport.h"

/*
    Useful documentation:
    - https://wiki.wireshark.org/Development/LibpcapFileFormat
    - http://www.tcpdump.org/linktypes.html
*/

#define DLT_USB_2_0 288

PcapExport::PcapExport(const char* filename)
{
    fh = fopen(filename, "wb");
    assert(fh);
    writeGlobalHeader();
}

PcapExport::~PcapExport(void)
{
    fclose(fh);
}

void PcapExport::writeGlobalHeader(void)
{
    pcap_hdr_t header;

    header.magic_number = 0xA1B23C4D;
    header.version_major = 2;
    header.version_minor = 4;
    header.thiszone = 0 /* Assume UTC */;
    header.sigfigs = 0;
    header.snaplen = 2048;
    header.network = DLT_USB_2_0;

    fwrite(&header, sizeof(pcap_hdr_t), 1, fh);
}

void PcapExport::writePacket(uint64_t ts, size_t len, void *data)
{
    pcaprec_hdr_t header;

    header.ts_sec = ts / 1000000000;
    header.ts_usec = ts % 1000000000;
    header.incl_len = len;
    header.orig_len = len;

    fwrite(&header, sizeof(pcaprec_hdr_t), 1, fh);
    fwrite(data, len, 1, fh);
}
