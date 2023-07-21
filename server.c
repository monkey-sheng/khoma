#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/socket.h>
// #include <linux/miscdevice.h>
#include <net/sock.h>
// #include <errno.h>
// #include <netdb.h>
// #include <netinet/tcp.h>
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <netinet/ip.h>
// #include <sys/mman.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <sys/uio.h>


#include "homa.h"
// #include "test_utils.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jason");
MODULE_DESCRIPTION("test homa server in the kernel.");

#define PORT 7777

/* extern int homa_socket(struct sock *sk);
extern int homa_bind(struct socket *sock, struct sockaddr *addr, int addr_len); */
/* more extern homa stuff here */

// static struct miscdevice *hello_dev, *hello_miscdev;


static int __init
server_init(void)
{
    // int error = misc_register(&hello_miscdev);
    // if (error)
    // {
    //     printk(KERN_ERR "%s misc_register() failed\n", __FUNCTION__);
    //     return -1;
    // }
    // else
    // {
    //     pr_info("misc_register %s\n", __FUNCTION__);
    // }
    // hello_dev = &hello_miscdev;
    printk(KERN_INFO "loaded kern homa server test\n");
    printk(KERN_INFO "Hello world!\n");

    char *server_homa_buf_region;

    // sock create
    struct socket *sock;
    int ret;
    ret = sock_create_kern(current->nsproxy->net_ns, AF_INET, SOCK_DGRAM, IPPROTO_HOMA, &sock);
    if (ret < 0)
        pr_err("%s, %d\n", "cannot create socket for homa in kernel!", ret);
    else
        pr_info("%s, %d\n", "sock_create success!", ret);

    // setsockopt
    pr_info("start setsockopt in kernel\n");
    struct homa_set_buf_args arg;
    int bufsize = 64 * HOMA_BPAGE_SIZE;
    server_homa_buf_region = kzalloc(bufsize, GFP_KERNEL);  // TODO: DON'T use kmalloc if bufsize is large
    pr_notice("server_homa_buf_region: %p\n", server_homa_buf_region);
    arg.start = server_homa_buf_region;
    arg.length = bufsize;

    ret = sock->ops->setsockopt(sock, IPPROTO_HOMA, SO_HOMA_SET_BUF, KERNEL_SOCKPTR(&arg), sizeof(arg));
    if (ret < 0)
    {
        pr_err("%d, %s\n", ret, "cannot setsockopt for homa in kernel!");
        return ret;
    }
    else
    {
        pr_info("%s %d\n", "setsockopt finished with", ret);
    }

    // recvmsg blocking (as server)
    /* Control blocks for receiving messages. */
    struct homa_recvmsg_args *recv_args;
    struct msghdr recv_hdr;
    recv_args = kmalloc(sizeof(*recv_args), GFP_KERNEL);
    memset(recv_args, 0, sizeof(*recv_args));
    pr_info("after memset, recv_args pad: %u, %u\n", recv_args->_pad[0], recv_args->_pad[1]);

    /* Address of message sender. */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = kernel_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        pr_err("%s %d, ret: %d\n", "bind failed on port", PORT, ret);
        return ret;
    }
    else
        pr_info("%s", "bind success");

    recv_hdr.msg_control = recv_args; // points to homa_recvmsg_args struct
    recv_hdr.msg_controllen = sizeof(*recv_args);

    // blocking listen for client message, use non blocking flag `HOMA_RECVMSG_NONBLOCKING` if needed
    recv_args->flags |= HOMA_RECVMSG_REQUEST;
    // recv_args->flags |= HOMA_RECVMSG_NONBLOCKING;
    recv_args->num_bpages = 0; // no bpage to return here

    // no need for kernel_recvmsg() for homa, iov stuff are not used by homa
    // the following two lines are literally just from kernel_recvmsg(), with a line for iov stuff removed
    recv_hdr.msg_control_is_user = false;
    recv_hdr.msg_name = kmalloc(sizeof(struct sockaddr_in), GFP_KERNEL); // client addr will be stroed by homa here
    recv_hdr.msg_namelen = sizeof(struct sockaddr_in);
    pr_info("%s\n", "calling sock_recvmsg()...");
    // TODO: kern_recvmsg
    struct kvec unused_kvec;
    pr_notice("before kernel_recvmsg(), recv_hdr.msg_control = recv_args addr: %p\n", recv_args);
    pr_notice("before kernel_recvmsg(), &recv_hdr: %p\n", &recv_hdr);
    pr_notice("recv_hdr: %p, recv_hdr.msg_name: %p\n", &recv_hdr, &recv_hdr.msg_name);
    int length = kernel_recvmsg(sock, &recv_hdr, &unused_kvec, 1, 1000, 0); // kvecs (can't be null), and flags in msghdr are unused by homa
    // int length = sock_recvmsg(sock, &recv_hdr, 0); // flags in msghdr are unused by homa
    if (length < 0) {
        pr_err("kernel_recvmsg error: %d\n", length);
        return length;
        // return 0;
    }
    else
        pr_info("kernel_recvmsg returned %d\n", length);

    // forget zero copy for now, just copy everything from bpages into contiguous memory
    char *recvbuf = kzalloc(length, GFP_KERNEL);
    pr_info("kmalloc done, start copying into recvbuf: %p\n", recvbuf);
    size_t offset = 0;
    size_t tocopy_len = length;
    pr_info("tocopy_len: %lu\n", tocopy_len);
    struct homa_recvmsg_args *recvd_args = recv_hdr.msg_control;
    pr_notice("recvd_args %p, recv_args: %p\n", recvd_args, recv_args);
    pr_notice("after recvmsg, recv_hdr.msg_controllen: %lu\n", recv_hdr.msg_controllen);
    pr_notice("after recvmsg, recvd_args has %u bpages\n", recvd_args->num_bpages);
    pr_notice("recvd_args: flags %d, cookie %llu, id %llu, pad %u, %u\n", recvd_args->flags, recvd_args->completion_cookie, recvd_args->id, recvd_args->_pad[0], recvd_args->_pad[1]);
    pr_notice("after recvmsg, recv_args has %d bpages\n", recv_args->num_bpages);
    pr_notice("id: %llu, cookies: %llu, flags: %d, \n", recv_args->id, recv_args->completion_cookie, recv_args->flags);
    for (uint32_t i = 0; i < recvd_args->num_bpages; i++)
    {
        pr_info("copying page %d, offset: %lu\n", i, offset);
        size_t len = ((tocopy_len > HOMA_BPAGE_SIZE) ? HOMA_BPAGE_SIZE : tocopy_len);
        pr_info("len: %lu, recv_args->bpage_offsets[i]: %u\n", len, recvd_args->bpage_offsets[i]);
        memcpy(recvbuf + offset, server_homa_buf_region + recvd_args->bpage_offsets[i], len);
        offset += len;
        tocopy_len -= len;
    }
    pr_info("%s %s\n", "copied message:", recvbuf);
    kvfree(recvbuf);
    pr_info("kvfree(recvbuf);\n");
    kfree(recv_args);
    kfree(recv_hdr.msg_name);
    pr_info("kfree(recv_hdr.msg_name;\n");

    pr_notice("server sock_release()\n");
    sock_release(sock);
    pr_notice("server sock_release() finished\n");

    kvfree(server_homa_buf_region);
    pr_info("%s\n", "freed homa buffer");
    return 0;
}

static void __exit
server_exit(void)
{

    printk(KERN_INFO "unloaded server module\n");
}

module_init(server_init);
module_exit(server_exit);
