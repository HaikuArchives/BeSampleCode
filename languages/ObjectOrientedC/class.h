
#ifndef H_CLASS
#define H_CLASS

#include <OS.h>

/*
 * base class definition
 */
typedef struct myBaseClass
{
	char		       *name;
	int                 id;
	struct myBaseClass *next;
} myBaseClass;

/*
 * inherited class definition.  Note inheritance through aggregation of the base class
 * as the first member of the struct.
 */
typedef struct myInheritedClass
{
	myBaseClass base;
	uint32      public_data;
	void        *private_data;
	void        (*public_action)(struct myInheritedClass *);
	void        (*private_action)(struct myInheritedClass *); 
} myInheritedClass;

/*
 * allocator and deallocator prototypes
 */
extern myInheritedClass *new_one(int);
extern myInheritedClass *new_two(int);
extern myInheritedClass *new_three(int);
extern void             delete_one(myInheritedClass *);
extern void             delete_two(myInheritedClass *);
extern void             delete_three(myInheritedClass *);

#endif
