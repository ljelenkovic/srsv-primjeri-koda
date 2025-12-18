#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MSG_QNAME    "/msgq_example_name"
#define MSG_MAXMSGS  5
#define MSG_MAXMSGSZ 20

#define MSG_MSGSZ MSG_MAXMSGSZ /* must be at least MSG_MAXMSGSZ */

int producer()
{
	mqd_t mqdes;
	struct mq_attr attr;
	char msg_ptr[] = "example message";
	size_t msg_len = strlen (msg_ptr) + 1;
	unsigned msg_prio = 10;

	attr.mq_flags = 0;
	attr.mq_maxmsg = MSG_MAXMSGS;
	attr.mq_msgsize = MSG_MAXMSGSZ;
	mqdes = mq_open(MSG_QNAME, O_WRONLY | O_CREAT, 00600, &attr);
	if (mqdes == (mqd_t) -1) {
		perror("producer:mq_open");
		return -1;
	}
	if (mq_send(mqdes, msg_ptr, msg_len, msg_prio)) {
		perror("mq_send");
		return -1;
	}
	printf("Sent: %s [prio=%d]\n", msg_ptr, msg_prio);

	return 0;
}

int consumer()
{
	mqd_t mqdes;
	char msg_ptr[MSG_MSGSZ];
	size_t msg_len;
	unsigned msg_prio;

	mqdes = mq_open(MSG_QNAME, O_RDONLY);
	if (mqdes == (mqd_t) -1) {
		perror("consumer:mq_open");
		return -1;
	}
	msg_len = mq_receive(mqdes, msg_ptr, MSG_MSGSZ, &msg_prio);
	if (msg_len < 0) {
		perror("mq_receive");
		return -1;
	}
	printf("Received: %s [prio=%d]\n", msg_ptr, msg_prio);

	return 0;
}

int main(void)
{
	producer();

	sleep (1);
	if (!fork()) {
		consumer();
		exit(0);
	}
	wait(NULL);

	mq_unlink(MSG_QNAME);

	return 0;
}
