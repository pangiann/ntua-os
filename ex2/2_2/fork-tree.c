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
#define SLEEP_TREE_SEC 6
#define SLEEP_PROC_SEC 10

void create_fork_tree(struct tree_node *node);

int main(int argc, char *argv[])
{
	struct tree_node *root;

	pid_t pid;
	int status;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
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
	  change_pname(root->name);
		create_fork_tree(root);
		exit(0);
	}

	sleep(SLEEP_TREE_SEC);
	show_pstree(pid);

	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}



void create_fork_tree(struct tree_node *node)
{
	pid_t pid;
	int status, i;

	printf("Node with name %s: Creating %d children...\n", node->name, node->nr_children);
	for (i = 0; i < node->nr_children; i++) {

		pid = fork();
		if (pid < 0) {
			perror("error while forking");
			exit(1);
		}
		if (pid == 0) {
			change_pname((node->children + i)->name);
			if ((node->children+i)->nr_children != 0) {
				/* MIDDLE NODE */
				create_fork_tree(node->children+i);
				exit(17);

			}
			else {
				/* LEAF NODE */
				printf("Node %s, is sleeping now...\n", (node->children + i)->name);
				sleep(SLEEP_PROC_SEC);

				printf("Node %s: Woke up and exiting...\n",  (node->children + i)->name);
				exit(42);
			}
		}
	}

	for (i = 0; i < node->nr_children; i++) {
		pid = wait(&status);
		explain_wait_status(pid, status);
	}

	exit(17);


}
