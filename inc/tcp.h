#ifndef __TCP_H_
#define __TCP_H_

/* type 0 ->client , 1 ->server */
int tcp_init(int type, const char *ip, int port);
int tcp_free(int fd);
int tcp_recv(int fd, char *_buf, unsigned int _size, int _s, int _u);
int tcp_send(int fd, char *_buf, unsigned int _size, int _s, int _u);
int tcp_accept(int fd, int _s, int _u);

#endif
