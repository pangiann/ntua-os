#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  20
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	/*
	 * initial process is A.
	 */
	pid_t pB, pC, pD, p;
	int status;

	change_pname("A");
	fprintf(stderr, "A process , PID = %ld: Creating child B process...\n", (long)getpid());

	/* A process creating two childs */
	pB = fork();
	if (pB < 0) {
		perror("A: error while forking to B");
		exit(1);
	}
	if (pB == 0) {
		/* In child (B) process */

		/*printf("B: Sleeping...\n");*/
	/*	sleep(SLEEP_PROC_SEC);*/

		/* Creating D process */
		pD = fork();
		if (pD < 0) {
			perror("B: error while forking to D");
			exit(1);
		}
		if (pD == 0) {
			/* In child (D) process */
			change_pname("D");
			printf("D: Sleeping...\n");
			sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
			exit(13);
		}
		/* Print the process tree root at pid */
		/*show_pstree(pB);*/
		/* for ask2-signals */
		/* kill(pid, SIGCONT); */
		/* Wait for the root of the process tree to terminate */
		change_pname("B");
		printf("Child process B, PID = %ld: Created child D with PID = %ld, waiting for it to terminate...\n",
        (long)getpid(), (long)pD);
		pD = wait(&status);
		explain_wait_status(pD, status);
		exit(19);
	}

	fprintf(stderr, "A process , PID = %ld: Creating child C process...\n", (long)getpid());
	pC = fork();
	if (pC < 0) {
		perror("A: error while forking to C");
		exit(1);
	}
	if (pC == 0) {
		change_pname("C");
		printf("C: Sleeping...\n");
		sleep(SLEEP_PROC_SEC);

		printf("C: Exiting...\n");
		exit(17);
	}

	/*show_pstree(pid)*/

	printf("Parent A, PID = %ld: Created childs C and B with pB = %ld and pC = %ld, waiting for them to terminate...\n", (long)getpid(), (long)pB, (long)pC);

	p = wait(&status);
	explain_wait_status(p, status);
	p = wait(&status);
	explain_wait_status(p, status);

	/*printf("A: Exiting...\n");*/
	exit(16);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
	pid_t pid;
	int status;


	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs();
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	/* wait_for_ready_children(1); */

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(getpid());

	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
