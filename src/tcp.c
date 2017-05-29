/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * COPYRIGHT NOTICE. 
 * Copyright (c), 2015 Lierda.
 * All rights reserved.
 * 
 * @file tcp.
 * @brief 
 * 
 * @version 1.0
 * @author au
 * @date 2015/06/15-15:31:37
 * @email dongliang@lierda.com
 * 
 * @revision:
 *  - 1.0 2015/06/15 by au.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <errno.h>

#include "tcp.h"

int tcp_init(int type, const char *ip, int port) {
	int 				reuse = 1;
	struct sockaddr_in 	sa;
	int 				ret;
	int fd;

	ret = socket(AF_INET, SOCK_STREAM, 0);
	if (ret < 0) {
		return -1;
	}
	fd = ret;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	//TODO: set more, if need

	if (type == 0) { //client
		sa.sin_family = AF_INET;
		sa.sin_port   = htons((short)port);
		inet_aton(ip, &(sa.sin_addr));
		ret = connect(fd, (struct sockaddr *)&sa, sizeof(sa));
		if (ret < 0) {
			return -2;
		}
	} else if (type == 1) {
		sa.sin_family = AF_INET;
		sa.sin_port   = htons((short)port);
		inet_aton(ip, &(sa.sin_addr));
		//sa.sin_addr.s_addr = INADDR_ANY;
		ret = bind(fd, (struct sockaddr *)&sa, sizeof(sa));
		if (ret < 0) {
			tcp_free(fd);
			return -3;
		}

		ret = listen(fd, 5);
		if (ret != 0) {
			tcp_free(fd);
			return -4;
		}
	}
	return fd;

}
int tcp_free(int fd) {
	if (!(fd > 0)) {
		return 0;
	}
	struct linger {
		int   l_onoff;
		int    l_linger;
	};
	char buf[128];
	struct linger linger = { 1, 0}; 
	setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *) &linger, sizeof(linger));
	shutdown(fd, SHUT_RDWR);
	do { 
		int ret = recv(fd, buf, sizeof(buf), 0);
		if (ret == 0)  break;
		if (ret == -1) {	
			if (errno == EAGAIN) break;
			if (errno == EWOULDBLOCK) break;
			if (errno == 9) break;
		}
	} while (1);
	close(fd);
	fd = -1;
	return 0;
}
int tcp_recv(int fd, char *_buf, unsigned int _size, int _s, int _u) {
	fd_set	fds;
	struct timeval	tv;
	int ret;
	int en;
	char es[256];

	if (_buf == NULL || _size <= 0 || _s < 0 || _u < 0 || fd <= 0)  {
		return -1;
	}
recv_select_tag:
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = _s;
	tv.tv_usec = _u;
	ret = select(fd + 1, &fds, NULL, NULL, &tv);
	if (ret < 0) {
		en = errno;
		if (en == EAGAIN || en == EINTR) {
			goto recv_select_tag;
		} else {
			strerror_r(en, es, sizeof(es));
			return -2;
		}
	} else if (ret == 0) {
		return 0;
	} else if (FD_ISSET(fd, &fds)) {
recv_tag:
		ret = recv(fd, _buf, _size, 0);
		if (ret < 0) {
			en = errno;
			if (en == EAGAIN || en == EINTR) {
				goto recv_tag;
			} else {
				return -3;
			}
		} else {
			if (ret == 0) {
				return -4; //remote close the socket
			} else {
				;
			}
		}
	} else {
		return -5;
	}
	return ret;
}
int tcp_send(int fd, char *_buf, unsigned int _size, int _s, int _u) {
	fd_set	fds;
	struct timeval tv;
	int ret;
	int en;
	int try_count = 5;

	if (_buf == NULL || _size <= 0 || _s < 0 || _u < 0 || fd <= 0)  {
		return -1;
	}

send_select_tag:
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = _s;
	tv.tv_usec = _u;
	ret = select(fd + 1, NULL, &fds, NULL, &tv);
	if (ret < 0) {
		en = errno;
		if (en == EAGAIN || en == EINTR) {
			goto send_select_tag;
		} else {
			return -2;
		}
	} else if (ret == 0) {
		return 0;
	} else if (FD_ISSET(fd, &fds)) {
send_tag:
		ret = send(fd, _buf, _size, 0);
		if (ret < 0) {
			en = errno;
			if ((en == EAGAIN || en == EINTR) && (try_count-- >= 0)) {
				usleep(10);
				goto send_tag;
			} else {
				return -3;
			}
		} else if (ret != (int)_size) {
			return -4;
		} else {
			;
		}
	} else {
		return -5;
	}
	return ret;

}
int tcp_accept(int fd, int _s, int _u) {
	fd_set	fds;
	struct timeval	tv;
	int ret;
	struct sockaddr_in sa;
	socklen_t		sl = sizeof(sa);
	int en;

accept_select_tag:
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = _s;
	tv.tv_usec = _u;

	ret = select(fd + 1, &fds, NULL, NULL, &tv);
	if (ret < 0) {
		en = errno;
		if (en == EAGAIN || en == EINTR) {
			goto accept_select_tag;
		} else {
			return -1;
		}
	} else if (ret == 0) {
		return 0;
	} else if (FD_ISSET(fd, &fds)) {
accept_tag:
		ret = accept(fd, (struct sockaddr *)&sa, &sl);
		if (ret < 0) {
			en = errno;
			if (en == EAGAIN || en == EINTR) {
				goto accept_tag;
			} else {
				return -2;
			}
		} else{
			;
		}
	} else {
		return -3;
	}
	return ret;

}


