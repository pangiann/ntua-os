#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void create_fork_tree(struct tree_node *node)
{
	/*
	 * Start
	 */
	pid_t pid_arr[node->nr_children], pid;
	int status, i;

	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), node->name);
	for (i = 0; i < node->nr_children; i++) {
		pid_arr[i] = fork();
		if (pid_arr[i] < 0) {
			perror("error while forking");
			exit(1);
		}
		if (pid_arr[i] == 0) {
			change_pname((node->children+i)->name);
			if ((node->children+i)->nr_children != 0) {
          /* MIDDLE NODE */
          create_fork_tree(node->children+i);
          exit(17);
    	}
      else {
          /* LEAF NODE */
          printf("Node %s, is sleeping now...\n", (node->children + i)->name);
					raise(SIGSTOP);

          printf("Node %s: Woke up and exiting...\n",  (node->children + i)->name);
          exit(42);
      }
    }
		wait_for_ready_children(1);
  }
	raise(SIGSTOP);
	printf("PID = %ld, name = %s is awake\n",
		(long)getpid(), node->name);


	/*
	 * Suspend Self
	 */
	for (i = 0; i < node->nr_children; i++) {
		kill(pid_arr[i], SIGCONT);
    pid = wait(&status);
    explain_wait_status(pid, status);
  }

	exit(17);
	/* ... */
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

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc != 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		change_pname(root->name);
		create_fork_tree(root);
		exit(1);
	}
	/*
	 * Father
	 */
	wait_for_ready_children(1);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
