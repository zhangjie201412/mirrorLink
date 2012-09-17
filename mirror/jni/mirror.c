#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include <jni.h>
#include <android/log.h>
#define LOGI(...)	((void)__android_log_print(ANDROID_LOG_INFO, "-mirror-", __VA_ARGS__))
#define LOGE(...)	((void)__android_log_print(ANDROID_LOG_ERROR, "-mirror-", __VA_ARGS__))

struct transpata {
	double latitude;
	double longitude;
};

int sockfd;

JNIEXPORT jint JNICALL
Java_com_android_example_Transpata_connect(JNIEnv *env, jobject obj, jstring ipaddress, jint port)
{
//	int sockfd;
//	char send_buff[BUFF_SIZE];
	struct sockaddr_in servaddr;

	//get address buff
	char address[128];
	const jbyte *str;
	str = (*env)->GetStringUTFChars(env, ipaddress, NULL);
	if(str == NULL) {
		LOGE("string null");
		return -1;
	}
	LOGI("address: %s", str);
	sprintf(address, "%s", str);
	//init socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		LOGE("socket error %s", strerror(sockfd));
		return -1;
	}

	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	int sock_port = (int)port;
	servaddr.sin_port = htons(sock_port);
	if(inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
		LOGE("inet_pton error for %s", address);
		return -1;
	}
	//connect socket
	if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof servaddr) < 0) {
		LOGE("connect error");
		return -1;
	}

	(*env)->ReleaseStringUTFChars(env, ipaddress, str);

	return 0;
}

JNIEXPORT jint JNICALL
Java_com_android_example_Transpata_send(JNIEnv *env, jobject obj, jdouble lat, jdouble lng)
{
	struct transpata trans;
	char send_buff[200];
	char recv_buff[4] = "";
	int n, try = 0;
	trans.latitude = (double)lat;
	trans.longitude = (double)lng;
	memset(send_buff, 0, 200);
	memcpy(send_buff, &trans, sizeof trans);

again:
	if(write(sockfd, send_buff, 200) != 200)
		LOGE("write error");
	if((n = read(sockfd, recv_buff, 4)) > 0) {
		if(!memcmp(recv_buff, "RVOK", 4)) {
			LOGI("recv %s", recv_buff);
		} else{
			if(try++ < 10)
				goto again;
		}
	} else if(n != 4) {
		if(try++ < 10)
			goto again;
	} else if(n < 0) {
		LOGE("read error");
	}


	return 0;
}

