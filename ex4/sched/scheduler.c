#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"
#include "queue.h"
/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */


/*
 * SIGALRM handler
 */

queue *q;

static pid_t
sched_delete_node_by_pid(pid_t pid)
{

	printf("my pid = %d\n", pid);
	Node *tmp = q->front;
	while(tmp != NULL) {

		if (tmp->pid == pid)
			break;
		else
			tmp = tmp->next;
	}
	if (tmp == NULL) {
		perror("wront pid, try again");
		return -1;
	}

	delete_node(q, tmp);


	return pid;
}

static void
sigalrm_handler(int signum)
{
	if (signum != SIGALRM) {
		fprintf(stderr, "Internal error: Called for signum %d, not SIGALRM\n",
			signum);
		exit(1);
	}

	// stop process running
	printf("ALARM! %d seconds have passed.\n", SCHED_TQ_SEC);
	if (kill(q->front->pid, SIGSTOP) < 0) {
		perror("Sigstop signal error");
		exit(1);
	}
	// move it to the end
	move_to_end(q);


	// print queue to check that everything is okay
	display_queue(q->front);



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
		if (p == 0)
			break;

		explain_wait_status(p, status);

		if (WIFEXITED(status)) {
			/* A child has died normally */
			dequeue(q, q->front->pid);
			display_queue(q->front);
			printf("we have %d remaining processes\n", q->count);
			kill(q->front->pid, SIGCONT);
			printf("Parent: Received SIGCHLD, child is dead. Exiting.\n");
			alarm(SCHED_TQ_SEC);

		}

		if (WIFSIGNALED(status)) {

			// when we've informed for a sigchld kill , either from shell or from something else we delete this process from queue

      sched_delete_node_by_pid(p);

			alarm(SCHED_TQ_SEC);

			kill(q->front->pid, SIGCONT);

		}
		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */

			printf("Parent: Child has been stopped. Moving right along...\n");
			kill(q->front->pid, SIGCONT);

		}
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

int main(int argc, char *argv[])
{
	int nproc;


	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	 pid_t pid;
	 q = malloc(sizeof(queue));
	 init_queue(q);

	 for (int i = 1; i < argc; i++) {
		 pid = fork();

		 if (pid < 0) {
				 perror("main: fork");
				 exit(1);
		 }
		 if (pid == 0) {
			 /* Child */
				char *executable = argv[i];
				char *newargv[] = { executable, NULL, NULL, NULL };
				char *newenviron[] = { NULL };
				printf("I am %s, PID = %ld\n",
			 		argv[i], (long)getpid());
			 	printf("About to replace myself with the executable %s...\n",
			 		executable);
				raise(SIGSTOP);
			 	execve(executable, newargv, newenviron);

			 	/* execve() only returns on error */
			 	perror("execve");
			 	exit(1);
		 }
		 enqueue(q, pid, argv[i]);


	 }

	nproc = argc-1; /* number of proccesses goes here */
	display_queue(q->front);
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

	//start first child
	if(kill(q->front->pid, SIGCONT) < 0){
		 perror("First child Cont error");
		 exit(1);
  }

	 //in TQ seconds trigger alarm and go to alarm_handler
	alarm(SCHED_TQ_SEC);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
