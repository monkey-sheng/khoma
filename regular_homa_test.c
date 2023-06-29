#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <errno.h>
#include <homa.h>

int main(int argc, char *argv[])
{
    int homa_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_HOMA);
    if (homa_socket < 0)
    {
        printf("Couldn't open Homa socket: %s\n", strerror(errno));
    }

    // Set up buffer region.
    char *buf_region;
    buf_region = (char *)mmap(NULL, 1000 * HOMA_BPAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (buf_region == MAP_FAILED)
    {
        printf("Couldn't mmap buffer region: %s\n", strerror(errno));
        exit(1);
    }
    struct homa_set_buf_args arg;
    arg.start = buf_region;
    arg.length = 1000 * HOMA_BPAGE_SIZE;
    int status = setsockopt(homa_socket, IPPROTO_HOMA, SO_HOMA_SET_BUF, &arg,
                        sizeof(arg));
    if (status < 0)
    {
        printf("Error in setsockopt(SO_HOMA_SET_BUF): %s\n",
               strerror(errno));
        exit(1);
    }

    /* Control blocks for receiving messages. */
    struct homa_recvmsg_args recv_args;
    struct msghdr recv_hdr;

    /* Address of message sender. */
    sockaddr_in_union source_addr;

    
}
