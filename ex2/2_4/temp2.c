#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_TREE_SEC 0.1
#define SLEEP_LEAF 1
void create_fork_tree(struct tree_node *, int pd);

int main(int argc, char **argv){
    struct tree_node *root;
    int status, pfd[2], result;
    pid_t pid;

    if(argc != 2){
        fprintf(stderr,"Usage: %s [input file]\n", argv[0]);
        exit(1);
    }

    if(pipe(pfd) < 0){
        perror("Initial pipe creation");
        exit(1);
    }


    root = get_tree_from_file(argv[1]);

    pid = fork();
    if(pid < 0){
        perror("Initial fork error");
        exit(2);
    }
    if(pid == 0){
        close(pfd[0]);           //close read port for child
        create_fork_tree(root, pfd[1]);     //pass file descriptor for writing

                                //into the function for further use
        exit(0);
    }

    //parent waits 0.1 sec and prints the tree
    sleep(SLEEP_TREE_SEC);
    show_pstree(pid);

    //here parent waits till the result is ready and reads it
    close(pfd[1]); //close write port for parent
    if(read(pfd[0], &result, sizeof(int)) != sizeof(int)){
        perror("read pipe");
    }

    pid = wait(&status);
    explain_wait_status(pid, status);

    printf("Result: %d\n", result);
    return 0;
}

void create_fork_tree(struct tree_node *node, int pd)
{
    int i = 0;
    int status, result, temp;
    pid_t pid;

    change_pname(node->name);
    int fd[2*node->nr_children];

	  printf("Node with name %s: Creating %d children...\n", node->name, node->nr_children);
    for(i = 0; i < node->nr_children; i++) {
		    printf("Node with name %s: Creating pipe\n", (node->children+i)->name);
        //Open one pipe for every child. In each iteration
        //a pipe is opened.
        if(pipe(fd + 2*i) < 0){
            perror("pipe creation");
            exit(1);
        }
    		pid = fork();
    		if (pid < 0) {
    			perror("fork error");
    			exit(1);
    		}

        if(pid == 0){
            change_pname((node->children + i)->name);
            if((node->children + i)->nr_children == 0){
                /*---------------LEAF NODE-----------------*/
                sleep(SLEEP_LEAF);     //wait so the right tree is printed

                int num = atoi((node->children + i)->name);

                close(fd[2*i]);     //close read port
                printf("Node %s, sending integer now to parent...\n", (node->children + i)->name);

                if(write(fd[2*i + 1], &num, sizeof(int)) != sizeof(int)){
                    perror("write pipe");
                }

                exit(0);
            }
            else{
                create_fork_tree(node->children + i, fd[2*i + 1]); //pass write port
            }
        }
	}

    /*------------------INTERVAL NODE--------------------*/
    /*number = (int *) malloc (ptr->nr_children * sizeof(int));*/
	int val[node->nr_children];

    //calculations
	printf("Interval node %s: Receiving integer from child.\n", node->name);

    for(i = 0; i < node->nr_children; i++){
        if(read(fd[2*i], &temp, sizeof(temp)) != sizeof(temp)){
            perror("read pipe");
        }
        val[i] = temp;
    }

    switch(strcmp(node->name,"+")) {
        case 0:
            result = val[0] + val[1];
            break;
        default:
            result = val[0] * val[1];
    }

    printf("Interval node %s: Writing result to parent.\n", node->name);
    //pass the result into the process before here
    if(write(pd, &result, sizeof(int)) != sizeof(int)){
        perror("write pipe");
    }

    //wait for children to exit and exit printing their exit status
    for (i = 0; i < node->nr_children; i++){
        pid = wait(&status);
        explain_wait_status(pid, status);
    }
    exit(0);
}
