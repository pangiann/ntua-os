#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "queue-priority.h"

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
  q->rear_high = NULL;
}

void enqueue(queue *q, pid_t pid, const char *n)
{
  Node *tmp;
  tmp = safe_malloc(sizeof(Node));
  tmp->pid = pid;
  char *name_ptr = strdup(n);
  tmp->name = name_ptr;
	tmp->id = q->max;
  tmp->priority = 'l';
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

void change_to_high (queue *q, Node* prev)
{

	//first change to high priority
	if (q->front->priority == 'l') {
			// queue front
			if (prev == NULL) {
				q->front->priority = 'h';
				q->rear_high = q->front;
			}
			// middle node
			else {
				//last node
				Node *n = prev->next;
				if (prev->next == q->rear) {
					q->rear = prev;
					prev->next = NULL;
					n->next = q->front;
					q->front = n;
					q->rear_high = n;
				}
				else {
					prev->next = n->next;
					n->next = q->front;
					q->front = n;
					q->rear_high = n;
				}
				n->priority = 'h';
			}
	}
	//we already have high priorities
	else {

		// first node of low priorities
	  if (q->rear_high == prev) {
	    prev->next->priority = 'h';
			q->rear_high = prev->next;

		}
	  else {
			//last node
			Node *n = prev->next;
	    if (n == q->rear) {
	      q->rear = prev;
	      prev->next = NULL;
	      n->next = q->rear_high->next;
	      q->rear_high->next = n;
	      q->rear_high = n;
	    }
			// middle node
	    else {
	      prev->next = n->next;
	      n->next = q->rear_high->next;
	      q->rear_high->next = n;
	      q->rear_high = n;
	    }
			n->priority = 'h';
		}
  }

}


void renew_rear_high (queue *q)
{
	// no high priorities
	if (q->front->priority == 'l')
		q->rear_high = NULL;
	else {
		Node *rear_high = q->front;
		while (rear_high->next != NULL && rear_high->next->priority != 'l')
			rear_high = rear_high->next;

		q->rear_high = rear_high;
	}

}

void change_to_low (queue *q, Node *prev)
{

	// only one node
	if (q->front == q->rear) {
		q->front->priority = 'l';
	}
	// first node
	else if (prev == NULL) {
		Node *n = q->front;
		q->front = n->next;
		n->next = NULL;
		q->rear->next = n;
		q->rear = n;
		n->priority = 'l';
	}
	// middle node
	else {
		Node *n = prev->next;
		prev->next = n->next;
		n->next = NULL;
		q->rear->next  = n;
		q->rear = n;
		n->priority = 'l';
	}
	renew_rear_high(q);

}
void delete_node(queue *q, Node *prev)
{

	// delete node
	
	// only one node
	if (q->front == q->rear)  {
		Node *tmp = q->front;
		tmp->next = NULL;
		free(tmp);
		q->front = NULL;
		q->rear = NULL;
		free(q);
	}
	// first node
	else if (prev == NULL) {
		Node *del_n = q->front;
		q->front = del_n->next;
		del_n->next = NULL;
		free(del_n);
	}
	//last node
	else if (q->rear == prev->next) {
		Node *del_n = prev->next;
		prev->next = del_n->next;
		del_n->next = NULL;

		q->rear = prev;
		free(del_n);
	}
	// middle node
	else {
		Node *del_n = prev->next;
		prev->next = del_n->next;
		del_n->next = NULL;
		free(del_n);
	}
  q->count--;
  if (q->count == 0) {
    printf("You are the mastermind, you did it.\nBye\n");
    exit(0);
  }
	renew_rear_high(q);

}
void dequeue(queue *q, pid_t pid)
{
  Node *tmp;
  tmp = q->front;
  q->front = q->front->next;
	tmp->next = NULL;
  q->count--;
  free(tmp);

  if (q->count == 0) {
    printf("You are the mastermind, you did it.\nBye\n");
    exit(0);
  }
	renew_rear_high(q);

}

void move_to_end(queue *q)
{

	// only one process in general
  if (q->front == q->rear) {
    return;
  }

  if (q->front->priority == 'l') {

    Node *tmp = q->front->next;
    q->front->next = NULL;
    q->rear->next = q->front;
    q->rear = q->front;
    q->front = tmp;
  }
  else {

		// only one high priority
		if (q->front->next->priority == 'l')
			return;

		Node *tmp = q->front->next;
    q->front->next = q->rear_high->next;
    q->rear_high->next = q->front;
		q->rear_high = q->front;
    q->front = tmp;
		if (q->rear->priority == 'h')
			q->rear = q->rear->next;

  }
	//display_queue(q->front);


}

void display_queue(Node *head)
{
  if (head == NULL)
    return;
  printf("pid = %d\tname = %s with id = %d and priority = %c\n", head->pid, head->name, head->id, head->priority);
  display_queue(head->next);
}
