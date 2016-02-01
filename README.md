# MemoryDebugger

This is a memory debugger to detect the following memory bugs right when it happens.

1. Underflow  
2. Overflow  
3. Access after Deletion  
4. Memory Leaks  

Memory bugs could be difficult to trace if we could not halt the program right when the access violation happens. This memory debugger solves this problem by using windows **VirtualAlloc** function to set a **PAGE_NOACCESS** zone. 

To detect overflow access violation, We allocate a page with half of them set to PAGE_NOACCESS, then any access into that region will trigger error and halt the program. Underflow detection could be done with similar technique.

The memory leaks will be reported with its line number and file names when they are allocated.

Besides debugging, it also generates statistics reports. See *Report Format* for more information.

## Usage
To integrate, it only requires at most three lines. Do the following steps, 

1. Put the **new_override.cpp** in your project.  
2. include **new_override.h** in your main program.  
3. At the beginning of *main()*, create a **LeakReporter** object.  
   It takes a c-style string as output file path. This object will report memory leak as well as allocation info when it is destructed. 
```c++
LeakReporter reporter("leaks.log");
```

## Configuration
To set the detection mode, one could use *SetDetectionMode* **BEFORE** the creation of LeakReporter. It is considered an error if detection mode is set after creation of LeakReporter.
```c++
    LeakReporter::SetDetectMode(LeakReporter::BoundaryDetectMode::DETECT_OVERFLOW);
```
Detection mode can be set to the followings,

1. DETECT_UNDERFLOW  
2. DETECT_OVERFLOW  
3. DETECT_NO_ACCESS_DETECTION  

When setting it to DETECT_NO_ACCESS_DETECTION, it is like the original allocation without access violation detection. It will still detects unallocated memory addresses.

Note that due to implementation restriction, we cannnot detect underflow and overflow at the same time.

## Report Format

If the program runs and finishes without crashing, the LeakReporter will
report memory leaks with the following information,

1. File, line number and function name when the undeleted allocation occurs
2. A statistics report that tells the user how many memory leaks there are for
every lines that have leaks.

Besides leak information, it will always report allocation information that
contains

1. A statistics report about how many allocations for all lines of code that
call new.  
2. A "hot chart" will be provided on the right side for each line in
the form of "star line". By comparing the length of the line, The user can 
easily identify if there are lines that
are memory allocation intensive.

## Comment
I override the 8 new() functions instead of using define. I made this decision
because it can also detect memory allocation inside STL containers. 

To track allocation, I use unordered_map. Although it is relatively
inefficient, it seems acceptable because the major purpose of the system is to
detect memory bug, and it will be disabled in the release build so it won't
affect the actual performance. 

I replace new() simply by overriding all 8 of them. Since most of their contents 
are the same, I write a helper function for allocation and deallocation, and
all new() and delete() just delegate their reponsibilities to these two functions.

## Others

Check out my other [portfolios](http://seanshih.com)!
