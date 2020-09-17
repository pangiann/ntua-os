#ifndef QUEUE_H
#define QUEUE_H

#include <unistd.h>

typedef struct Node_q {
  pid_t pid;
  int id;
  char *name;
  struct Node_q *next;
  char priority;
} Node;


typedef struct queue {
  int count;
  int max;
  Node *front;
  Node *rear;
  Node *rear_high;
} queue;




void init_queue(queue *q);

void enqueue(queue *q, pid_t pid, const char *name);

void delete_node(queue *q, Node *n);

void dequeue(queue *q, pid_t pid);

void move_to_end(queue *q);

void change_to_low (queue *q, Node *n);

void change_to_high (queue *q, Node* n);

void renew_rear_high (queue *q);

void display_queue(Node *head);

#endif
