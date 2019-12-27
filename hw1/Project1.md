# Thread Management on NachOS

<p align='right'>2019Fall OS HW1</p>
<p align='right'>EE4 B05901064 林承德 </p>
## Motivation

### Problem Analysis

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; It's not quite easy to understand the whole code of NachOS, therefore I searched the related website on Google. After reading some information, I found and acknowledged the main two problems as below:

* Incorrect PageTable

   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  When executing the process, the machine loads the only pagetable from the running process. Each process should have its specific and unique pagetable in order to make sure every physical pages in main memory are allocated to at most one process and have unique index of virtual page to the process.

    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; However, when initialing the pagetable in `addrspace.cc`, all of the processes get the same table that can access all the physical pages. Therefore, processes sharing all the memory together, resulting in the wrong outcome.

   

* Improper access to main memory due to wrong address

    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; When loading the program to main memory at `AddrSpace::Load()`, it puts in the location corresponding to the virtual address. However,  the main memory should be accessed by physical address.

### Dealing Problem

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; In order to know whether the physical pages are allocated to the process or not, we need a global table to record them. Moreover, the pagetable should only contain the physical pages allocated to the process. What's more, the virtual address needs to transform to the physical address when loading the program.



## Implementation

### Circular Queue

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; On the website, the author adds a static bitwise hash array at `class AddrSpace` to store the state of the physical pages. When allocating the memory to the process, the OS does a linear search from the physical address of 0 to find unused pages. However, the less the number of the physical page, the more the access frequency of the memory. This might cause the different lives on the same memory. Therefore, I use a FIFO queue that stores the physical address of the unused pages.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; The OS needs to take $O(|PhyPages|)$ to allocate a physical page to a process due to the shift of the value in queue. Since the number of the physical pages is constant, I implement the container of physical pages in circular queue that size is equal to the number. When first creating the address table, the OS will construct the circular queue with page number of all physical pages. The core of the code is shown below. As a result, the action of allocation and releasing both take $O(1)$ time.

```c++
class AddrSpace{
    static unsigned UnusedPhyPage[NumPhysPages]; // Circular Queue
    static unsigned start = 0, end = 0; // pointer
    static unsigned RemainPhyPages = 0; //size of queue
    static bool IsInit = false; // Queue has been initial or not
}

AddrSpace::AddrSpace() {
    if(!IsInit){ // Queue hasn't been initial
        for(unsigned i = 0; i < NumPhysPages; ++i)UnusedPhyPage[i] = i;
        IsInit = true;
    }
}
```




### Initializing the pagetable dependent on  the process

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Since we know how many pages do the process need only after it calls `AddrSpace::Load()`, we create the `Addrspace::pageTable` in `Load()` function instead of the constructor.  Before allocating the memory, OS needs to check the number of the pages that processes request is greater or equal to the numbers of unused pages(i.e. the size of circular queue), otherwise OS will raise a core dump. After that, we allocate the physical pages to the process and remove them from the circular queue. Since we only move the start pointer in queue, the spending time of the single allocation is $O(1)$.

```c++
ASSERT(numPages <= RemainPhyPages); // check if request is out of the remain memory
RemainPhyPages -= numPages;

pageTable = new TranslationEntry[numPages]; // create pageTable
for(unsigned i = 0; i < numPages; ++i){
    pageTable[i].virtualPage = i;
    pageTable[i].physicalPage = UnusedPhyPage[start];
    start = ( start == NumPhysPages-1 ) ? 0 : start+1;
}
```

### Returning the memory

Put the memory released by the process to the circular queue. We don't need to shift the value in queue, therefore the time complexity of single addition of queue is $O(1)$.

```c++
AddrSpace::~AddrSpace(){
   if(pageTable){ // check if pageTable exists
       for(unsigned i = 0; i < numPages; ++i){
           //append the page to circular queue
           UnusedPhyPage[end] = pageTable[i].physicalPage;
           end = (end == NumPhysPages-1) ? 0 : end+1;
       }
       RemainPhyPages += numPages;
       delete pageTable;
   }
}
```



### Transformation from virtual address to physical address

$\left\{\begin{array}{2}physpage = pageTable[virtaddr / PageSize].physPage \\ \\ offset = virtaddr \% PageSize \\ \\ physaddr = physpage * PageSize + offset\end{array}\right .$



## Result

$NumPhysPages = 32$

The used pages of both test1 and test2 is 11.

### test1 & test2

  <img src="D:\Workspace\nachos\hw1\success.jpg" style="zoom:100%;" />

**SUCCESS**



### test1 & test2*2

![](D:\Workspace\nachos\hw1\overflow.jpg)

**Successfully detect the overflow**

Core dumped due to overflow(all of the processes need 33 pages, which is greater than the total of physical pages).