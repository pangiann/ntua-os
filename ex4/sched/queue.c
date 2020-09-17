#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "queue.h"

void *safe_malloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL) {
		fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
			size);
		exit(1);
	}

	return p;
}

void init_queue(queue *q) {
  q->count = 0;
	q->max = 0;
  q->front = NULL;
  q->rear = NULL;
}

void enqueue(queue *q, pid_t pid, const char *n)
{
  Node *tmp;
  tmp = safe_malloc(sizeof(Node));
  tmp->pid = pid;
  char *name_ptr = strdup(n);
  tmp->name = name_ptr;
	tmp->id = q->max;
	q->max++;
  q->count++;
  tmp->next = NULL;
  if (q->rear != NULL) {
    q->rear->next = tmp;
    q->rear = tmp;
  }
  else
    q->front = q->rear = tmp;

}

void delete_node(queue *q, Node *n)
{
	Node *tmp = q->front;
	// only one process in queue
	if (n == tmp) {
		q->front = n->next;
		n->next = NULL;
		free(n);
	}
	else {
		while (tmp->next != n) tmp = tmp->next; //find previous node
		// node we want to delete is rear in queue
		if (n == q->rear) {
			q->rear = tmp;
			tmp->next = NULL;
			free(n);
		}
		else {
			tmp->next = n->next;
			n->next = NULL;
			free(n);
		}
	}
	q->count--;
	if (q->count == 0) {
    printf("You are the mastermind, you did it.\nBye\n");
    exit(0);
  }


}
void dequeue(queue *q, pid_t pid)
{
  Node *tmp;
  tmp = q->front;
	tmp->next = NULL;
  q->front = q->front->next;
  q->count--;
  free(tmp);

  if (q->count == 0) {
    printf("You are the mastermind, you did it.\nBye\n");
    exit(0);
  }
}

void move_to_end(queue *q)
{

	if (q->front == q->rear) {
		return;
	}
  Node *tmp = q->front->next;
  q->front->next = NULL;
  q->rear->next = q->front;
  q->rear = q->front;
  q->front = tmp;


}

void display_queue(Node *head)
{
  if (head == NULL)
    return;
  printf("pid = %d\tname = %s with id = %d\n", head->pid, head->name, head->id);
  display_queue(head->next);
}
