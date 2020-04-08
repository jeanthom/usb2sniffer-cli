#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "pcapexport.h"

#define CSR_ACCESSORS_DEFINED 1

extern "C" {
#include "parse.h"
#include "xbar/etherbone.h"
#include "xbar/csr.h"
#include "xbar/sdram_phy.h"
#include "xbar/xbar.h"
extern ftdev_t gfd;
}

enum FileFormat {
	PCAP,
	USB
};

static enum {
	STARTING,
	CAPTURING,
	TEARDOWN,
	IDLE
} g_captureState = STARTING;

enum CaptureSpeed {
	HS = 0,
	FS = 1,
	LS = 2,
};

void sigintHandler(int sig_num) {
	g_captureState = TEARDOWN;
}

int capture(CaptureSpeed speed, const char *device, const char *filename, FileFormat format) {
	ftdev_t fd;
	int ret;
	char *buf;
	char *pktbuf;
	int streamid;
	size_t len;
	uint32_t plen;
	uint8_t type;
	uint8_t val;
	uint64_t ts;
	uint8_t event;
	uint32_t drop_count;
	PcapExport *exp = nullptr;

	if (format == PCAP) {
		exp = new PcapExport(filename);
	}

	ret = ft60x_open(&fd, device);
	if (ret < 0) {
		fprintf(stderr, "Cannot open %s\n", device);
		return -1;
	}
	memcpy(&gfd, &fd, sizeof(ftdev_t));

	/* configure sdram */
	sdram_configure();

	/* flush fifo and cores */
	rst_manager_reset_write(1);

	/* ulpi switch */
	ulpi_sw_oe_n_out_write(0);
	ulpi_sw_s_out_write(0);

	/* ulpi init */
	ulpi_init(fd, speed);

	/* enable timer */
	iticore0_packer_time_enable_write(1);

	/* generate start pattern */
	iticore0_start_pattern_write(1);

	/* start capture */
	overflow0_reset_write(1);
	ulpi_enable(fd, 1);

	auto session = usb_new_session();

	pktbuf = (char *)malloc(2048);

	while (g_captureState != IDLE) {
		if (g_captureState == TEARDOWN) {
			/* stop capture */
			ulpi_enable(fd, 0);

			drop_count = overflow0_count_read();
			printf("Stopped: packet dropped count (ulpi): %d\n", drop_count);

			/* generate event stop */
			iticore0_packer_ev_event_write(USB_EVENT_STOP);
			iticore0_packer_ev_event_write(USB_EVENT_STOP);

			/* disable timer */
			iticore0_packer_time_enable_write(0);
		}

		streamid = ubar_recv_packet(fd, &buf, &len);
		if(streamid < 0) {
			printf("Capture device has been disconnected.\n");
			goto exit;
		} else if (streamid == 1) {
			usb_add_data(session, (uint8_t*)buf, len);

			while(usb_read_data(session, &type, &val, &ts));

			while(usb_read_packet(session, &type, (uint8_t*)pktbuf, &plen, &ts)) {
				if (g_captureState == CAPTURING && format == PCAP) {
					exp->writePacket(ts, plen, pktbuf);
				}
			}

			while(usb_read_event(session, &event)) {
				switch(event) {
				case USB_EVENT_STOP:
					g_captureState = IDLE;
					break;
				case USB_EVENT_START:
					g_captureState = CAPTURING;
					usb_reset_timestamp(session);
					break;
				default:
					break;
				}
			}
		}

		if (buf) {
			free(buf);
		}
	}

	if (format == USB) {
		FILE *fh = fopen(filename, "wb");
		usb_write_session(session, fh);
		fclose(fh);
	}

exit:
	delete exp;
	free(pktbuf);
	ft60x_close(fd);

	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <speed: ls/fs/hs> <device> <output file>\n", argv[0]);
		fprintf(stderr, "example: %s hs /dev/ft60x0 foobar.pcap\n", argv[0]);
		return EXIT_FAILURE;
	}

	FileFormat format = USB;
	std::string filename(argv[3]);
	auto idx = filename.rfind('.');
	if (idx != std::string::npos) {
		auto extension = filename.substr(idx+1);
		if (extension == "pcap") {
			format = PCAP;
		} else if (extension == "usb") {
			format = USB;
		} else {
			fprintf(stderr, "Unrecognized %s extension\n", extension);
			return EXIT_FAILURE;
		}
	} else {
		fprintf(stderr, "No file extension for output file (should be either .pcap or .usb)\n");
		return EXIT_FAILURE;
	}

	CaptureSpeed speed = LS;
	if (!strcmp(argv[1], "hs")) {
		speed = HS;
	} else if (!strcmp(argv[1], "fs")) {
		speed = FS;
	} else if (!strcmp(argv[1], "ls")) {
		speed = LS;
	} else {
		fprintf(stderr, "Unknown USB speed specified, should be either ls (Low Speed), fs (Full Speed) or hs(High Speed)\n");
		return EXIT_FAILURE;
	}

	signal(SIGINT, sigintHandler);

	int res = capture(speed, argv[2], argv[3], format);

	return (res == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}