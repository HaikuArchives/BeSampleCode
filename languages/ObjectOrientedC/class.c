

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "class.h"

/*
 * definition of the private data structures used by the "one", "two", and "three"
 * subclasses
 */
struct one_private_data
{
	int data;
};

struct two_private_data
{
	float data;
};

struct three_private_data
{
	char data[16];
};

/*
 * prototypes for the member functions used by each subclass
 */
static void public_action(struct myInheritedClass *);
static void one_private_action(struct myInheritedClass *);
static void two_private_action(struct myInheritedClass *);
static void three_private_action(struct myInheritedClass *);

/*
 * templates used to simplify creation of instances of each of the subclasses.
 * alternatively, once could simply assign values to each of the struct members
 * in the new_* functions.
 */
static myInheritedClass one = {
	{
		"one",
		0,
		0
	},
	1,
	0,
	public_action,
	one_private_action
};


static myInheritedClass two = {
	{
		"two",
		0,
		0
	},
	2,
	0,
	public_action,
	two_private_action
};


static myInheritedClass three = {
	{
		"three",
		0,
		0
	},
	3,
	0,
	public_action,
	three_private_action
};


/*
 * implementation of the member functions used to access the class' data
 */
static void public_action(struct myInheritedClass *mc)
{
	printf("class %s, instance id %d\npublic data is %ld\n", mc->base.name, mc->base.id, mc->public_data);
}


static void one_private_action(struct myInheritedClass *mc)
{
	struct one_private_data *pd = (struct one_private_data *) mc->private_data;

	printf("private_data is %d\n", pd->data);
}


static void two_private_action(struct myInheritedClass *mc)
{
	struct two_private_data *pd = (struct two_private_data *) mc->private_data;

	printf("private_data is %f\n", pd->data);
}


static void three_private_action(struct myInheritedClass *mc)
{
	struct three_private_data *pd = (struct three_private_data *) mc->private_data;

	printf("private_data is %s\n", pd->data);
}


/*
 * class instance allocators and deallocators
 */
myInheritedClass *new_one(int id)
{
	myInheritedClass *rc = 0;
	
	rc = (myInheritedClass *) malloc(sizeof(myInheritedClass));
	
	if(rc == 0)
		goto error;
	
	memcpy(rc, &one, sizeof(myInheritedClass));
	
	rc->base.id = id;
	
	rc->private_data = malloc(sizeof(struct one_private_data));
	
	if(rc->private_data == 0)
		goto error1;
	
	((struct one_private_data *) rc->private_data)->data = rand();
		
	return rc;
	
error1:
	free(rc);
error:
	return 0;
}


myInheritedClass *new_two(int id)
{
	myInheritedClass *rc = 0;
	
	rc = (myInheritedClass *) malloc(sizeof(myInheritedClass));
	
	if(rc == 0)
		goto error;
	
	memcpy(rc, &two, sizeof(myInheritedClass));
	
	rc->base.id = id;
	
	rc->private_data = malloc(sizeof(struct two_private_data));
	
	if(rc->private_data == 0)
		goto error1;
	
	((struct two_private_data *) rc->private_data)->data = (float) rand();
		
	return rc;
	
error1:
	free(rc);
error:
	return 0;
}


myInheritedClass *new_three(int id)
{
	myInheritedClass *rc = 0;
	
	rc = (myInheritedClass *) malloc(sizeof(myInheritedClass));
	
	if(rc == 0)
		goto error;
	
	memcpy(rc, &three, sizeof(myInheritedClass));
	
	rc->base.id = id;
		
	rc->private_data = malloc(sizeof(struct three_private_data));
	
	if(rc->private_data == 0)
		goto error1;
	
	sprintf(((struct three_private_data *) rc->private_data)->data, "0x%08lx", (uint32) rand());
		
	return rc;
	
error1:
	free(rc);
error:
	return 0;
}


void delete_one(myInheritedClass *mc)
{
	free(mc->private_data);
	free(mc);
}


void delete_two(myInheritedClass *mc)
{
	free(mc->private_data);
	free(mc);
}


void delete_three(myInheritedClass *mc)
{
	free(mc->private_data);
	free(mc);
}


