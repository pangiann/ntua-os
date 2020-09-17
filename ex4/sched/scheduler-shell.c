#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"
#include "queue.h"
/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */


queue *q;
/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	display_queue(q->front);
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
	Node *tmp = q->front;
	while(tmp != NULL) {

		if (tmp->id == id)
			break;
		else
			tmp = tmp->next;
	}
	if (tmp == NULL) {
		perror("wrong id, try again");
		return -1;
	}
	if (kill(tmp->pid, SIGKILL) < 0) {
		perror("Sigkill signal error");
		exit(1);
	}
	//delete_node(q, tmp);


	return id;
}


static pid_t
sched_kill_task_by_pid(pid_t pid)
{

	//printf("my pid = %d\n", pid);
	Node *tmp = q->front;
	while(tmp != NULL) {

		if (tmp->pid == pid)
			break;
		else
			tmp = tmp->next;
	}
	if (tmp == NULL) {
		perror("wrong pid, try again");
		return -1;
	}

	delete_node(q, tmp);


	return pid;
}

/* Create a new task.  */
static void
sched_create_task(char *executable)
{
		pid_t pid;
		pid = fork();

		if (pid < 0) {
				perror("main: fork");
				exit(1);
		}
		if (pid == 0) {
			/* Child */
			 char *newargv[] = { executable, NULL, NULL, NULL };
			 char *newenviron[] = { NULL };
			 //printf("I am %s, PID = %ld\n",
				 //executable, (long)getpid());
			 //printf("About to replace myself with the executable %s...\n",
				 //executable);
			 raise(SIGSTOP);
			 execve(executable, newargv, newenviron);

			 /* execve() only returns on error */
			 perror("execve");
			 exit(1);
		}
		enqueue(q, pid, executable);


}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

		default:
			return -ENOSYS;
	}
}

/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	if (signum != SIGALRM) {
		fprintf(stderr, "Internal error: Called for signum %d, not SIGALRM\n",
			signum);
		exit(1);
	}



	//printf("ALARM! %d seconds have passed.\n", SCHED_TQ_SEC);
	if (kill(q->front->pid, SIGSTOP) < 0) {
		perror("Sigstop signal error");
		exit(1);
	}
	move_to_end(q);


	//display_queue(q->front);



	/* Setup the alarm again */
	if (alarm(SCHED_TQ_SEC) < 0) {
		perror("alarm");
		exit(1);
	}

}

/*
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	pid_t p;
	int status;

	if (signum != SIGCHLD) {
		fprintf(stderr, "Internal error: Called for signum %d, not SIGCHLD\n",
			signum);
		exit(1);
	}

	/*
	 * Something has happened to one of the children.
	 * We use waitpid() with the WUNTRACED flag, instead of wait(), because
	 * SIGCHLD may have been received for a stopped, not dead child.
	 *
	 * A single SIGCHLD may be received if many processes die at the same time.
	 * We use waitpid() with the WNOHANG flag in a loop, to make sure all
	 * children are taken care of before leaving the handler.
	 */

	for (;;) {
		p = waitpid(-1, &status, WUNTRACED | WNOHANG);
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}
	//	printf("p = %d\n", p);

		if (p == 0)
			break;

		//explain_wait_status(p, status);

		if (WIFEXITED(status)) {
			/* A child has died */
			dequeue(q, q->front->pid);
			//display_queue(q->front);
			//printf("we have %d remaining processes\n", q->count);
			kill(q->front->pid, SIGCONT);
			//printf("Parent: Received SIGCHLD, child is dead. Exiting.\n");
			alarm(SCHED_TQ_SEC);

		}
		if (WIFSIGNALED(status)) {
			sched_kill_task_by_pid(p);
			alarm(SCHED_TQ_SEC);
			kill(q->front->pid, SIGCONT);


		}
		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */

			//printf("Parent: Child has been stopped. Moving right along...\n");
			kill(q->front->pid, SIGCONT);

		}
	}
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
	}
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	enqueue(q, p, SHELL_EXECUTABLE_NAME);
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
	return p;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int nproc;
	pid_t pid;
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	q = malloc(sizeof(queue));
	init_queue(q);
	/* Create the shell. */

	/* TODO: add the shell to the scheduler's tasks */



	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	for (int i = 1; i < argc; i++) {
		 pid = fork();

		 if (pid < 0) {
				 perror("main: fork");
				 exit(1);
		 }
		 if (pid == 0) {
			 /* Child */
				char executable[] = "prog";
				char *newargv[] = { executable, NULL, NULL, NULL };
				char *newenviron[] = { NULL };
				//printf("I am %s, PID = %ld\n",
			 		//argv[i], (long)getpid());
			 	//printf("About to replace myself with the executable %s...\n",
			 		//executable);
				raise(SIGSTOP);
			 	execve(executable, newargv, newenviron);

			 	/* execve() only returns on error */
			 	perror("execve");
			 	exit(1);
		 }
		 enqueue(q, pid, argv[i]);


	}


	nproc = argc; /* number of proccesses goes here */

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

	// start shell
	//if (kill(pshell, SIGCONT) < 0) {
	//	perror("Shell cont error");
	//	exit(1);
	//}
	//start first child
	display_queue(q->front);
	if(kill(q->front->pid, SIGCONT) < 0){
		 perror("First child Cont error");
		 exit(1);
	}


	 //in TQ seconds trigger alarm and go to alarm_handler
  alarm(SCHED_TQ_SEC);
	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
