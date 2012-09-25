#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>  /* inet(3) functions */
#include <string.h>
#include <sys/stat.h>   /* for S_xxx file mode constants */
#include <sys/uio.h>            /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>             /* for Unix domain sockets */
#include <sys/epoll.h>

#include "global.h"

struct descripState {
	int control[2];
	int remot;
	int local;
};



static struct descripState _desState[1];

static int make_block(int fd)
{
	int ret, flag;

	flag = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flag & (~O_NONBLOCK));
}

static int
epoll_register( int  epoll_fd, int  fd, int block)
{
    struct epoll_event  ev;
    int                 ret, flags;

    if(block) {
	    /* important: make the fd non-blocking */
	    flags = fcntl(fd, F_GETFL);
	    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    ev.events  = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret;
}

static int
epoll_deregister( int  epoll_fd, int  fd )
{
    int  ret;
    do {
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_DEL, fd, NULL );
    } while (ret < 0 && errno == EINTR);
    return ret;
}



static void
remot_thread(void* arg)
{
	struct descripState* state = (struct descripState* )arg;
	int remote_fd = state->remot;
	int n;
	struct transpata trans;
	char buff[MAX_SIZE];
	while(1) {
		if((n = read(remote_fd, buff, MAX_SIZE)) > 0) {
			bzero(&trans, sizeof trans);
			//write to control
			if(write(state->control[0], buff, MAX_SIZE) != MAX_SIZE)
				printf("remote error: write error\n");

		} else if(n < 0)
			perror("remote error");
	}
}

int main(void)
{
	int listen_fds[2];
	int local_connfd = -1;
	int remot_connfd = -1;
	struct descripState* s = _desState;
	//[0] -----local server fd
	//[1] -----remot server fd
	int listenfd = -1;
	struct sockaddr_in servaddr;
	int ret = -1;
	//-------local server-------
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0) {
		printf("local error: socket anyway\n");
		return -1;
	}
	bzero(&servaddr, sizeof servaddr);
	servaddr.sin_family	= AF_INET;
	servaddr.sin_addr.s_addr= htonl(INADDR_ANY);
	servaddr.sin_port	= htons(LOCAL_PORT);
	
	ret = bind(listenfd, (struct sockaddr *)&servaddr,
			sizeof servaddr);
	if(ret < 0) {
		printf("local error: when bind\n");
		return -1;
	}
	ret = listen(listenfd, 10);
	if(ret < 0) {
		printf("remote error: when listen\n");
	}
	listen_fds[0] = listenfd;
	//---------remote server-------
	listenfd = -1;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0) {
		printf("remote error: socket anyway\n");
		return -1;
	}
	bzero(&servaddr, sizeof servaddr);
	servaddr.sin_family	= AF_INET;
	servaddr.sin_addr.s_addr= htonl(INADDR_ANY);
	servaddr.sin_port	= htons(REMOT_PORT);
	
	ret = bind(listenfd, (struct sockaddr *)&servaddr,
			sizeof servaddr);
	if(ret < 0) {
		printf("remote error: when bind\n");
		return -1;
	}

	ret = listen(listenfd, 10);
	if(ret < 0) {
		printf("remote error: when listen\n");
	}
	listen_fds[1] = listenfd;
	//------------end
	// socket pair 
	s->control[0] = -1;
	s->control[1] = -1;

	if(socketpair(AF_LOCAL, SOCK_STREAM, 0, s->control) < 0) {
		printf("error: can not create thread control socket pair\n");
		return -1;
	}
	//----------add listen fd into epoll event
	int epoll_fd = epoll_create(10);
	int control_fd = s->control[1];
	epoll_register(epoll_fd, listen_fds[0], 1);
	epoll_register(epoll_fd, listen_fds[1], 1);
	epoll_register(epoll_fd, control_fd, 1);
	
	for(;;) {
		struct epoll_event events[10];
		int ne, nevents;

		nevents = epoll_wait(epoll_fd, events, 10, -1);
		if(nevents < 0) {
			if(errno != EINTR)
				printf("epoll wait() unexpected error\n");
			continue;
		}
		printf("thread received %d events\n", nevents);
		for(ne = 0; ne < nevents; ne++) {
			if((events[ne].events & (EPOLLERR | EPOLLHUP)) != 0) {
				printf("EPOLLERR or EPOLLHUP\n");
				return -1;
			}
			if((events[ne].events & EPOLLIN) != 0) {
				int fd = events[ne].data.fd;
				pthread_t local_pid, remot_pid;
				if(fd == listen_fds[0]) {
					local_connfd = accept(fd, (struct sockaddr *)NULL,
							NULL);
					if(local_connfd < 0) {
						printf("local error: when accept\n");
						return -1;
					} else {
						printf("local accept done\n");
					}
					s->local = local_connfd;
					//create a thread to do with local works
					//....
		//			ret = pthread_create(&local_pid, NULL, local_thread, s);
				} else if(fd == listen_fds[1]) {
					remot_connfd = accept(fd, (struct sockaddr *)NULL,
							NULL);
					if(remot_connfd < 0) {
						printf("remote error: when accept\n");
						return -1;
					} else {
						printf("remote accept done\n");
					}
					s->remot = remot_connfd;
					//create a thread to do with remote works
					//....
					
					ret = pthread_create(&remot_pid, NULL, remot_thread, s);
				} else if(fd == control_fd) {
					//read from control[0]
					//....
					char buff[MAX_SIZE];
					struct transpata trans;
					char recv_ok[4] = "RVOK";
					int n;
					bzero(&trans, sizeof trans);
					memcpy(&trans, buff, sizeof trans);

					if((n = read(fd, buff, MAX_SIZE)) > 0) {
						if(!memcmp(buff, "SMAG", 4)) {
							printf("recive: x = %f, y = %f, z = %f\n", trans.mx, trans.my, trans.mz);
							//write to control
//							if(write(s->local, buff, 32) != 32)
//								printf("remote error: write error\n");
						} else if(!memcmp(buff, "SGPS", 4)) {
							make_and_send(s->local, trans.longitude, trans.latitude, 0, 0, 0);
						}
						if(write(s->remot, recv_ok, 4) != 4)
							printf("remote error: write error\n");

					} else if(n < 0)
						perror("remote error");

				}
			}
		}

	}
	return 0;
}

