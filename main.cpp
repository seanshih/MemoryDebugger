#include <iostream>
#include <vector>
#include <cstdio>
#include "new_override.h"

// OPTIONAL test - do NOT enable if you use #define new
#define ENABLE_NOTHROW_TEST

// Scenarios
// BEGIN: DO NOT MODIFY THE FOLLOWING 13 FUNCTION

/*****************************************************************************/
/* Memory Leaks */
// DO NOT MODIFY
double* project2_leaks_helper() {
	return new double;
}

// DO NOT MODIFY
void project2_leaks() {
	int* ints[10] = {NULL};
	// 10 allocations - 9 deletes
	for (int i = 0 ; i < 10 ; i++) {
		ints[i] = new int;
	}
	for (int i = 0 ; i < 9 ; i++) {
		delete ints[i];
		ints[i] = NULL;
	}

	// 10 vector allocations - 9 vector deletes
	for (int i = 0 ; i < 10 ; i++) {
		ints[i] = new int[i+1];
	}
	for (int i = 0 ; i < 9 ; i++) {
		delete[] ints[i];
		ints[i] = NULL;
	}

	// 1 allocation through helper function
	double* d = project2_leaks_helper();

	// All total - 3 leaks
}

/*****************************************************************************/
/* Overflows */

// DO NOT MODIFY
void project2_writeoverflow() {
	int* ints = new int[10];
	// Valid Access
	for (int i = 0 ; i < 10 ; i++) {
		ints[i] = i;
	}

	// Write overflow
	for (int i = 1 ; i <= 10 ; i++) {
		ints[i] = ints[i-1];
	}

	delete[] ints;
}

// DO NOT MODIFY
void project2_readoverflow() {
	int* ints = new int[10];
	// Valid Access
	for (int i = 0 ; i < 10 ; i++) {
		ints[i] = i;
	}

	// Read overflow
	for (int i = 0 ; i <= 10 ; i++) {
		printf("ints %d is %d\n", i, ints[i]);
	}

	delete[] ints;
}

/*****************************************************************************/
/* Deleted Memory Access */

// DO NOT MODIFY
void project2_deletedmemorywrite() {
	int* i = new int;
	*i = 42;
	delete i;

	// Write Access to Deleted Pointer
	*i = 43;
}

// DO NOT MODIFY
void project2_deletedmemoryread() {
	int* i = new int;
	*i = 42;
	delete i;

	// Read Access to Deleted Pointer
	printf("Deleted i is: %d\n", *i);
}

/*****************************************************************************/
/* Double Delete */

// DO NOT MODIFY
void project2_doubledelete() {
	int* i = new int;
	delete i;

	// Double Delete
	delete i;
}

// DO NOT MODIFY
void project2_doublevectordelete() {
	int* i = new int[2];
	delete[] i;

	// Double Delete
	delete[] i;
}

/*****************************************************************************/
/* Mismatched new/delete[] and new[]/delete */

// DO NOT MODIFY
void project2_newvectordelete() {
	int* ints = new int[2];

	// Mismatched new[]/delete
	delete ints;
}

// DO NOT MODIFY
void project2_newdeletevector() {
	int* ints = new int;

	// Mismatched new/delete[]
	delete[] ints;
}

/*****************************************************************************/
/* Random Pointer Delete */

// DO NOT MODIFY
void project2_randompointer1() {
	int local_int = 0;
	int* i = &local_int;

	// Cannot delete a stack variable
	delete i;
}

// DO NOT MODIFY
void project2_randompointer2() {
	int* i = new int;

	// Move the pointer forward by 1 byte - no longer i
	int* bad_i = (int*)((char*)i + 1);

	// Cannot delete a pointer that was not returned from new
	delete bad_i;
}

/*****************************************************************************/
/* GOOD Usage - There is nothing wrong with this test */

// DO NOT MODIFY
void project2_good() {
	// Simple Allocation Test
	int* i = new int;
	*i = 42;
	delete i;
	i = NULL;

	// Simple Vector Allocation Test
	i = new int[2];
	i[0] = 0;
	i[1] = i[0] + 1;
	delete[] i;
	i = NULL;

	// Simple std Container Test
        std::vector<int> v;
	v.push_back(1);
	v.push_back(2);
	v.push_back(3);

	// delete NULL test
	i = NULL;
	delete i;

	// Large array test - ~16K
	i = new int[8000];
	memset(i, 0x00, 8000 * sizeof(int));
	memset(i, 0xFF, 8000 * sizeof(int));
	delete[] i;
	i = NULL;
	
	// Very large array test - ~4MB
	i = new int[1024*1024];
	memset(i, 0x00, 1024*1024 * sizeof(int));
	memset(i, 0xFF, 1024*1024 * sizeof(int));
	delete[] i;
	i = NULL;

	// Zero byte allocation must be non-NULL and Unique
	i = new int[0];
	int* i2 = new int[0];
	// If this breakpoint triggers, you will fail Scenario 12
	if (i == i2) { __debugbreak(); }
	delete[] i;
	delete[] i2;

	// bad_alloc test
	bool bad_alloc_caught = false;
	try {
		// a 4GB request is simply not serviceable
		size_t s = (size_t)0xF0000000;
		char* c = new char[s];
	} catch (std::bad_alloc) { bad_alloc_caught = true; }
	// If this breakpoint triggers, you will fail Scenario 12
	if (!bad_alloc_caught) { __debugbreak(); }

	// nothrow test
	// To enable this test, uncomment the #define at the top of this file
	// You are NOT required to run this test
	// Because it uses the placement new syntax, it will break #define new
	// However, you should implement a conformant nothrow new regardless
#ifdef ENABLE_NOTHROW_TEST
	bad_alloc_caught = false;
	try {
		// a 4GB request is simply not serviceable
		size_t s = (size_t)0xF0000000;
		char* c = new(std::nothrow) char[s];
		if (c != NULL) { __debugbreak(); }
	} catch (std::bad_alloc) { bad_alloc_caught = true; }
	// If this breakpoint triggers, you will fail Scenario 12
	if (bad_alloc_caught) { __debugbreak(); }
#endif // ENABLE_NOTHROW_TEST
}

// END: DO NOT MODIFY THE PREVIOUS 13 FUNCTION

/*****************************************************************************/
/* Framework */

void scenario_underflow_read()
{
    int* inta = new int;
    int q = *(inta - 1);
    (void)q;
    delete inta;
}

void scenario_underflow_write()
{
    int* inta = new int;
    *(inta - 1) = 1;
    delete inta;
}


// Pass 1-12 to choose the scenario
// IE: project2.exe 5
int main(int argc, char *argv[]) 
{
    int scenario = 0;
    if (argc > 1)
        scenario = std::atoi(argv[1]);

    // set detection mode to either detecting overflow/underflow, or no access violation detection
    LeakReporter::SetDetectMode(LeakReporter::BoundaryDetectMode::DETECT_OVERFLOW);
    LeakReporter reporter("leaks.log");

    if (scenario == 13) { scenario_underflow_read(); return 0; }
    if (scenario == 14) { scenario_underflow_write(); return 0; }

	// Test Harness
	// BEGIN: DO NOT MODIFY THE FOLLOWING 15 LINES
	if (argc > 1) { sscanf_s(argv[1], "%d", &scenario); }
	switch (scenario) {
		case 12: project2_good(); break;
		case 11: project2_randompointer2(); break;
		case 10: project2_randompointer1(); break;
		case 9:  project2_newdeletevector(); break;
		case 8:  project2_newvectordelete(); break;
		case 7:  project2_doublevectordelete(); break;
		case 6:  project2_doubledelete(); break;
		case 5:  project2_deletedmemoryread(); break;
		case 4:  project2_deletedmemorywrite(); break;
		case 3:  project2_readoverflow(); break;
		case 2:  project2_writeoverflow(); break;
		default: project2_leaks(); break;
	}
	// END: DO NOT MODIFY THE PREVIOUS 15 LINES

	return 0;
}
