#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <linux/if_link.h>
#include <linux/if_xdp.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define VETH_NAME "veth1"
#define SOCKET_NAME "/tmp/sock_cal_bpf_fd"
#define QUEUE 0
#define RING_SIZE 1024

void cleanup() {
	unlink(SOCKET_NAME);
	exit(EXIT_FAILURE);
}

#define handle_error(msg) { fprintf(stderr, "%s %s(%d)\n", msg, strerror(errno), errno); cleanup(); }


int main()
{
	int ifindex = if_nametoindex(VETH_NAME);
	if (ifindex == 0) {
		handle_error("Unable to get ifindex");
	}

	int sock = socket(AF_XDP, SOCK_RAW | SOCK_CLOEXEC, 0);
	//int sock = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (sock < 0) {
		handle_error("open socket failed");
	}

	char* umem_buf = aligned_alloc(4096, 4096 * 10); //dumb but easy
	if(umem_buf == 0){
		handle_error("malloc failed");
	}
	struct xdp_umem_reg umem = {.addr = umem_buf, .len = 4096 * 10, .chunk_size=2048, .headroom=0};

	if(setsockopt(sock, SOL_XDP, XDP_UMEM_REG,& umem, sizeof(umem))){
		handle_error("setting umem failed");
	}
	int fill_ring_size = RING_SIZE;
	if(setsockopt(sock, SOL_XDP, XDP_UMEM_FILL_RING, &fill_ring_size, sizeof(int)) < 0){
		handle_error("setting fill ring failed");
	}
	int com_ring_size = RING_SIZE;
	if(setsockopt(sock, SOL_XDP, XDP_UMEM_COMPLETION_RING, &com_ring_size, sizeof(int)) < 0){
		handle_error("setting completion ring failed");
	}
	int tx_ring_size = RING_SIZE;
	if(setsockopt(sock, SOL_XDP, XDP_TX_RING, &tx_ring_size, sizeof(int)) < 0){
		handle_error("setting tx ring failed");
	}
	int rx_ring_size = RING_SIZE;
	if(setsockopt(sock, SOL_XDP, XDP_RX_RING, &rx_ring_size, sizeof(int)) < 0){
		handle_error("setting rx ring failed");
	}






	struct sockaddr_xdp sxdp = {};
        sxdp.sxdp_family = PF_XDP; 
	sxdp.sxdp_ifindex = ifindex;
	sxdp.sxdp_queue_id = QUEUE;
	sxdp.sxdp_flags = XDP_USE_NEED_WAKEUP;

	/*
	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = 50500;
	//strcpy(my_addr.sun_path, "/tmp/foo");
	// */
	printf("Binding %d to ifindex %d queue %d\n", sock, ifindex, QUEUE);
	if (bind(sock, (struct sockaddr *)&sxdp, sizeof(struct sockaddr_in))) {
	//if (bind(sock, (struct sockaddr *)&sxdp, sizeof(struct sockaddr_xdp))) {
		handle_error("bind socket failed");
	}

	return 0;

}
