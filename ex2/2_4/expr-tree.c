#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "proc-common.h"
#include "tree.h"
#define SLEEP_TREE_SEC 1
#define SLEEP_PROC_SEC 10
#define SLEEP_INTERV 2

void create_fork_tree(struct tree_node *node, int wfd);
int main(int argc, char *argv[])
{
	struct tree_node *root;



	pid_t pid;
	int status;
	int result;
	int pfd[2];
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	if(pipe(pfd) < 0) {
        perror("Initial pipe creation");
        exit(1);
    }

	root = get_tree_from_file(argv[1]);
	print_tree(root);


	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* child */
		close(pfd[0]);
	    change_pname(root->name);
		create_fork_tree(root, pfd[1]);
		exit(0);
	}

  sleep(SLEEP_TREE_SEC);

	show_pstree(pid);

	close(pfd[1]);

	if(read(pfd[0], &result, sizeof(int)) != sizeof(int)){
        perror("read pipe");
    }
	pid = wait(&status);
	explain_wait_status(pid, status);
	printf("result = %d\n", result);
	return 0;
}



void create_fork_tree(struct tree_node *node, int wfd)
{
	pid_t pid_arr[node->nr_children], pid;
	int status, i;


	printf("Node with name %s: Creating %d children...\n", node->name, node->nr_children);
	int pfd[2];
	printf("Node with name %s: Creating pipe\n", node->name);
	if(pipe(pfd) < 0) {
		perror("pipe");
		exit(1);
	}

	for (i = 0; i < node->nr_children; i++) {

		pid_arr[i] = fork();
		if (pid_arr[i] < 0) {
			perror("error while forking");
			exit(1);
		}
		if (pid_arr[i] == 0) {
			change_pname((node->children+i)->name);
			if ((node->children+i)->nr_children != 0) {
				/* ---------- INTERVAL NODE ----------------- */
				create_fork_tree(node->children+i, pfd[1]);
				exit(17);
			}
			else {
				/* ----------- LEAF NODE -------------------- */


				sleep(SLEEP_PROC_SEC);
				printf("Node %s, sending integer now to parent...\n", (node->children + i)->name);
				int num = atoi((node->children + i)->name);
				close(pfd[0]);
				if (write(pfd[1], &num, sizeof(int)) != sizeof(int)) {
					perror("child: write to pipe");
					exit(1);
				}
				/*
				printf("Node %s: Woke up and exiting...\n",  (node->children + i)->name);
				*/
				exit(42);
			}
		}
	}

	/* -------------------------- INTERVAL NODE ------------------------*/


	/* -------------------------- READING VALUES -----------------------*/
	int val[node->nr_children];
	printf("Interval node %s: Receiving integer from child.\n", node->name);
	if (read(pfd[0], &val[0], sizeof(int)) != sizeof(int)) {
		perror("parent: read from pipe");
		exit(1);
	}
	if (read(pfd[0], &val[1], sizeof(int)) != sizeof(int)) {
		perror("parent: read from pipe");
		exit(1);
	}

	/* -------------------------- WRITING RESULT -----------------------*/

	int result;
	printf("Interval node %s: Writing result to parent.\n", node->name);
	switch(strcmp(node->name,"+")) {
        case 0:
            result = val[0]+val[1];
            break;
        default:
            result = val[0]*val[1];
  }
	if (write(wfd, &result, sizeof(result)) != sizeof(result)) {
		perror("interval node write result to pipe");
		exit(1);
	}

	for (i = 0; i < node->nr_children; i++) {
		pid = wait(&status);
		explain_wait_status(pid, status);
	}

}
