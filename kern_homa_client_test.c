#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/socket.h>
#include <net/sock.h>
#include "homa.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jason");
MODULE_DESCRIPTION("test homa client in the kernel.");

#define SERVER_PORT 7777

char *client_homa_buf_region;

static int __init
client_init(void)
{
    printk(KERN_INFO "loaded kern homa client test\n");
    // sock create
    struct socket *sock;
    int ret;
    ret = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_HOMA, &sock);
    if (ret < 0)
        pr_err("%s, %d\n", "cannot create socket for homa in kernel!", ret);
    else
        pr_info("%s, %d\n", "sock_create success!", ret);

    // setsockopt
    pr_info("%s\n", "start setsockopt in kernel");
    struct homa_set_buf_args arg;
    int bufsize = 64 * HOMA_BPAGE_SIZE;
    client_homa_buf_region = vzalloc(bufsize); // TODO: DON'T use kmalloc if bufsize is large
    arg.start = client_homa_buf_region;
    arg.length = bufsize;

    ret = sock->ops->setsockopt(sock, IPPROTO_HOMA, SO_HOMA_SET_BUF, KERNEL_SOCKPTR(&arg), sizeof(arg));
    if (ret < 0)
    {
        pr_err("%d, %s\n", ret, "client cannot setsockopt for homa in kernel!");
        return ret;
    }
    else
    {
        pr_info("%s %d\n", "client setsockopt finished with", ret);
    }

    struct homa_sendmsg_args send_args;
    memset(&send_args, 0, sizeof(send_args));
    struct msghdr send_hdr;

    // destination address
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_port = htons(SERVER_PORT);
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    send_args.completion_cookie = 87654321;
    send_args.id = 0; // request should specify id = 0, will be set by homa

    send_hdr.msg_name = &target_addr;
    send_hdr.msg_namelen = sizeof(target_addr);
    send_hdr.msg_control = &send_args;
    send_hdr.msg_controllen = sizeof(send_args);
    send_hdr.msg_control_is_user = false;

    /* char msg_content[] = "ping from client!\n";
    struct iovec msg_iov = {.iov_base = msg_content, .iov_len = sizeof(msg_content)};
    struct iov_iter iter;
    iov_iter_init(&iter, WRITE, &msg_iov, 1, sizeof(msg_content));  // total bytes same as size of struct (bc only the struct)
    send_hdr.msg_iter = iter;
    // send_hdr.msg_iovlen = 1;
    send_hdr.msg_control_is_user = 0; */

    // ret = sock_sendmsg(sock, &send_hdr);
    char msg_string1[] = "this is a message to be sent. this is the second sentence in message\n";
    char msg_string2[] = "this is the second line in message\n";
    struct kvec msg_content1 = {.iov_base = msg_string1, .iov_len = sizeof(msg_string1)};
    struct kvec msg_content2 = {.iov_base = msg_string2, .iov_len = sizeof(msg_string2)};
    struct kvec msg_tosend[2] = {msg_content1, msg_content2};
    ret = kernel_sendmsg(sock, &send_hdr, msg_tosend, 1, sizeof(msg_string1));
    if (ret < 0)
        pr_err("kernel_sendmsg error: %d\n", ret);
    else
        pr_info("kernel_sendmsg returned %d\n", ret);

    pr_info("closing client socket\n");
    sock_release(sock);
    return 0;
}

static void __exit
client_exit(void)
{
    kvfree(client_homa_buf_region);
    pr_info("%s", "freed homa buffer\n");
    // if (hello_dev)
    //     misc_deregister(hello_dev);
    printk(KERN_INFO "unloaded client module\n");
}

module_init(client_init);
module_exit(client_exit);