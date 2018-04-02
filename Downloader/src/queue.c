
#include "queue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct node{     //node structure for the queue to be used as a linked list
	struct node* next;
	void* data;
} Node;

typedef struct QueueStruct {
	Node* first;
	Node* last;
	sem_t room_taken;     //takes care of the amount of room taken in the queue
	sem_t room_available; //takes care of the amount of room left in the queue
	pthread_mutex_t queue_handler; //keeps threads from accessing the queue more than one at a time
} Queue;

Node* make_node(void* item){         //makes a node for when adding to the queue 
	Node* n = malloc(sizeof(Node));
	n->data = item;
	n->next = NULL;
	return n;
}

Queue *queue_alloc(int size) {
	Queue *queue = malloc(sizeof(Queue));      
	queue->first = NULL;
	queue->last = NULL;
	pthread_mutex_init(&(queue->queue_handler), NULL);  //intitialise the mutex
	sem_init(&(queue->room_available), 0, size); //initialise the available semaphore to the size of the empty queue (size)
	sem_init(&(queue->room_taken), 0, 0);        //is necessary for the semaphores to be sepcific to the queue item itself as otherwise multiple queues might use the same semaphore (and mutex)
	return queue;
}

void queue_free(Queue *queue) {
	while(queue->first){                        //only need to free the queue if it has contents 
		Node* next_node = queue->first->next;   //preparing the second item in the queue to replace the first
		free(queue->first->data);
		free(queue->first);                     
		queue->first = next_node;               //make what was the second item now the first 
	}
	free(queue);                                //free the initially allocated queue
}

void queue_put(Queue *queue, void *item) {
	sem_wait(&(queue->room_available));  		   //decriment the available semaphore as we put one item in the queue, makes sure that if there is no room available
										                                                                   //then the semaphore won't allow more things in the queue
	pthread_mutex_lock(&(queue->queue_handler));   //make sure only one thread is in the crit region at any one time
	if (!queue->first) {                  		   //if the queue is empty
		Node* first_node = make_node(item);   
		queue->first = first_node;
		queue->last = first_node;                  //the last node is also the first one if the queue has only one node
	}
	else{
		Node* new_last = make_node(item); //making a node for the put data to become the future last node of the queue
		queue->last->next = new_last;     //making the past last node to now be linked to the new last node
		queue->last = new_last;           //setting the queue's last node to refer to the new last node
	} 
	pthread_mutex_unlock(&(queue->queue_handler)); 
	sem_post(&(queue->room_taken));      		  //incriment the taken semaphore as there has been a node put into the queue
}

void *queue_get(Queue *queue) {
	sem_wait(&(queue->room_taken));       //decriments the taken semaphore as if we take something out 
										  //of the queue we will have less room in the queue being taken up
	pthread_mutex_lock(&(queue->queue_handler));   //mutex to make sure two threads don't try to take the same task
	Node* front = queue->first;           //make sure to keep the front of the queue in order to free it later
	void* item = front->data;      		  //get the data from the first node of the queue
	queue->first = queue->first->next;           //set the queue's first node to equal the node after the one we just got
	pthread_mutex_unlock(&(queue->queue_handler));
	sem_post(&(queue->room_available));            //as something was taken from the queue we have more room
	free(front);                          //free the front node as it is now cut off from the linked list and we will be unable to access it after this funciton call
	return item;
}

