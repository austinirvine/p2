/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.

  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comp)(const void *, const void *))
{
	q->front = NULL;
	q->cmp = comp;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	node a;
	a.data = ptr;
	a.next = NULL;
	int retv = 0;
	// if its the first job, make it front
	if (q->front == NULL){
		q->front = &a;
	} else {
		retv = 1;
		node * cur_node = (node *)q->front;
		node * pre_node = (node *)q->front;
		while(cur_node != NULL) {
			// If we get back the first point, we know that a needs to be before cur_node
			if((q->cmp)(a.data, cur_node->data) == -1){
				// If its the first node, we need a special case
				if (cur_node == q->front){
					q->front = &a;
					a.next = cur_node;
				} else {
					pre_node->next = &a;
					a.next = cur_node;
				}
				break;
			}
			// iterate the nodes
			pre_node = cur_node;
			cur_node = cur_node->next;
			retv++;
		}
		// cmp never returned a, so were at the end of the queue and need to have a special case
		if (cur_node == NULL){
			pre_node->next = &a;
		}
	}
	return retv;
}

/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	void * retv;
	if (q->front == NULL) {
		retv = NULL;
	} else {
		retv = q->front->data;
	}
	return retv;
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	node * retv;
	if (q->front == NULL) {
		retv = NULL;
	} else {
		retv = q->front;
		q->front = retv->next;
		retv->next = NULL;
	}
	return (retv == NULL)? NULL : retv->data;
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	node * retv;
	if (q->front == NULL || index > priqueue_size(q)){
		retv = NULL;
	} else {
		node * n = q->front;
		int i = 0;
		while(i != index) {
			n = n->next;
			i += 1;
		}
		retv = n;
	}
	return (retv == NULL)? NULL : retv->data;
}


/**
  Removes all instances of ptr from the queue.

  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	int retv = 0;
	if(q->front == NULL)
	{
		retv = 0;
	} else if (q->front->next == NULL && q->front == ptr){
		retv = 1;
	} else {
		node * cur_node = q->front;
		node * pre_node = q->front;
		while (cur_node != NULL) {
			if(cur_node->data == ptr) {
				pre_node->next = cur_node->next;
				cur_node->next = NULL;
				free(cur_node);
				cur_node = pre_node->next;
				retv += 1;
			}
			pre_node = cur_node;
			cur_node = cur_node->next;
		}
	}
	return retv;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	node retv;
	if(q->front == NULL || index > priqueue_size(q))
	{
		retv.data = NULL;
	} else if (q->front->next == NULL && index == 0){
		retv = *q->front;
	} else {
		node * cur_node = q->front;
		node * pre_node = q->front;
		for (int i = 0; i < index; i++){
			pre_node = cur_node;
			cur_node = cur_node->next;
		}
		pre_node->next = cur_node->next;
		cur_node->next = NULL;
		retv = *cur_node;
	}
	return retv.data;
}


/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	int retv = 0;
	if(q->front == NULL) {
		retv = 0;
	} else {
		node * cur_node = q->front;
		retv = 1;
		while(cur_node->next != NULL) {
			cur_node = cur_node->next;
			retv += 1;
		}
	}
	return retv;
}


/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	if(priqueue_size(q) != 0) {
		node * cur_node = q->front;
		node * pre_node = q->front;
		while(cur_node != NULL) {
			cur_node = cur_node->next;
			free(pre_node);
			pre_node = cur_node;
		}
		q->front = NULL;
	}
}
