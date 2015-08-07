#ifndef DEBUG_H
#define DEBUG_H

#define BOCHS_BREAK asm volatile ("xchg %bx, %bx")



#define ASSERT(val) if(!(val)) { \
	printf("ASSERT '%s' FAILED IN FILE '%s' AT LINE '%d'\n", #val, __FILE__, __LINE__); \
	asm volatile ("cli\nhlt"); \
}

#define assert(val) ASSERT(val)

#endif