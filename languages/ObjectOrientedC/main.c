
#include <stdio.h>
#include <stdlib.h>
#include "class.h"

static myInheritedClass *make_class(int id);

/*
 * this function returns a random inherited class instance
 */
static myInheritedClass *make_class(int id)
{
	myInheritedClass *rc = 0;
	
	switch(rand() % 3)
	{
		case 0:
			rc = new_one(id);
		break;
		
		case 1:
			rc = new_two(id);
		break;
		
		case 2:
			rc = new_three(id);
		break;
	}
	return rc;
}


int main(int argc, char **argv)
{
	myInheritedClass *head = 0, *tmp = 0;
	int i;
	
	/*
	 * make a list of 5 radom subclass instances.
	 */
	for(i=0;i<5;i++)
	{
		tmp = make_class(i);
		if(tmp != 0)
		{
			tmp->base.next = (myBaseClass *) head;
			head = tmp;
		}
	}
	
	/*
	 * iterate through the list and call the member function to
	 * print out each subclasses public and private data.  Whee, polymorphism!
	 */
	for(tmp = head; tmp != 0; tmp = (myInheritedClass *) tmp->base.next)
	{
		printf("{\n");
		tmp->public_action(tmp);
		tmp->private_action(tmp);
		printf("}\n\n");
	}
	
	return 0;
}

