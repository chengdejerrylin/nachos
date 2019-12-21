// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "filesys.h"
#include "machine.h"
#include "noff.h"
#include "disk.h"
#include <climits>

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace(){
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   if(pageTable){
       kernel->machine->memmgr->ReleaseAll(pageTable, numPages);
       delete pageTable;
   }
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
//	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;
    size = numPages * PageSize;

    pageTable = new TranslationEntry[numPages];

    int codeVaddr    = noffH.code.virtualAddr;
    int codeFileAddr = noffH.code.inFileAddr;
    int codeSize     = 0;
    int dataVaddr    = noffH.initData.virtualAddr;
    int dataFileAddr = noffH.initData.inFileAddr;
    int dataSize     = 0;

    for(unsigned i = 0; i < numPages; ++i){
        ASSERT(kernel->machine->memmgr->AcquirePage(pageTable, i) );

        if(i == codeVaddr / PageSize && codeSize < noffH.code.size) {
            DEBUG(dbgAddr, "Initializing code segment.");
            int phyaddr = pageTable[i].physicalPage * PageSize + codeVaddr % PageSize;
            int wrsize = PageSize - codeVaddr % PageSize;
            if (codeSize + wrsize > noffH.code.size) wrsize = noffH.code.size - codeSize;

            DEBUG(dbgAddr, codeVaddr << ", " << wrsize);
            executable->ReadAt(&(kernel->machine->mainMemory[phyaddr]), wrsize, codeFileAddr);

            codeVaddr    += wrsize;
            codeFileAddr += wrsize;
            codeSize     += wrsize;
        }

        if(i == dataVaddr / PageSize && dataSize < noffH.initData.size) {
            DEBUG(dbgAddr, "Initializing data segment.");
            int phyaddr = pageTable[i].physicalPage * PageSize + dataVaddr % PageSize;
            int wrsize = PageSize - dataVaddr % PageSize;
            if (dataSize + wrsize > noffH.initData.size) wrsize = noffH.initData.size - dataSize;

            DEBUG(dbgAddr, dataVaddr << ", " << wrsize);
            executable->ReadAt(&(kernel->machine->mainMemory[phyaddr]), wrsize, dataFileAddr);

            dataVaddr    += wrsize;
            dataFileAddr += wrsize;
            dataSize     += wrsize;
        }

    }
    

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

// then, copy in the code and data segments into memory
	// if (noffH.code.size > 0) {
    //     DEBUG(dbgAddr, "Initializing code segment.");
	// DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
    //     	executable->ReadAt(
	// 	&(kernel->machine->mainMemory[pageTable[noffH.code.virtualAddr/PageSize].physicalPage * PageSize + noffH.code.virtualAddr % PageSize]), 
	// 		noffH.code.size, noffH.code.inFileAddr);
    // }
	// if (noffH.initData.size > 0) {
    //     DEBUG(dbgAddr, "Initializing data segment.");
	// DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
    //     executable->ReadAt(
	// 	&(kernel->machine->mainMemory[pageTable[noffH.initData.virtualAddr/PageSize].physicalPage * PageSize + noffH.initData.virtualAddr % PageSize]),
	// 		noffH.initData.size, noffH.initData.inFileAddr);
    // }

    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
    if (!Load(fileName)) {
	cout << "inside !Load(FileName)" << endl;
	return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {
    // if(kernel->machine->pageTableSize){
    //     pageTable=kernel->machine->pageTable;
    //     numPages=kernel->machine->pageTableSize;
    // }
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}

FrameInfoEntry::FrameInfoEntry(): valid(false), lock(false), pageTable(0), vpn(0) {}

MemoryManager::MemoryManager() : count(0){
    frameTable = new FrameInfoEntry[NumPhysPages];
    swapTable  = new FrameInfoEntry[NumSectors];
}

MemoryManager::~MemoryManager(){
    delete [] frameTable;
    delete [] swapTable;
}

bool MemoryManager::AcquirePage(TranslationEntry *pageTable, int vpn) {
    for (size_t i = 0; i < NumPhysPages; ++i) {
        if(!frameTable[i].valid){
            SetNewPage(pageTable, vpn, i);
            return true;
        }
    }

    // no free page
    int page = SavePage();
    if(page == -1) return false;  
    
    SetNewPage(pageTable, vpn, page);
    return true;
}

void MemoryManager::ReleaseAll(TranslationEntry *pageTable, int num){
    int total = 0;
    for (size_t i = 0; i < num; ++i){   
        if(pageTable[i].valid && frameTable[pageTable[i].physicalPage].valid){
            frameTable[pageTable[i].physicalPage].valid = false;
            ++total;
        }
    }

    if(total == num)return;

    for (size_t i = 0; i < NumSectors; i++){
        if(swapTable[i].valid && swapTable[i].pageTable == pageTable){
            swapTable[i].valid = false;
            ++total;
            if(total == num)return;
        }
    }

    ASSERTNOTREACHED();
}

bool MemoryManager::AccessPage(TranslationEntry *pageTable, int vpn){
    if(pageTable[vpn].valid){
        frameTable[pageTable[vpn].physicalPage].count = count++;
        return true;
    }

    int ppn = -1;
    for(size_t i = 0; i < NumPhysPages; ++i){
        if(!frameTable[i].valid){
            ppn = i;
            break;
        }
    }
    if(ppn == -1)ppn = SavePage();
    ASSERT(ppn != -1);
    ASSERT(RestorePage(pageTable, vpn, ppn));

    frameTable[pageTable[vpn].physicalPage].count = count++;
    return true;
}

void MemoryManager::SetNewPage(TranslationEntry *pageTable, int vpn, int ppn){
    pageTable[vpn].virtualPage = vpn;
    pageTable[vpn].physicalPage = ppn;
    pageTable[vpn].valid = true;
    pageTable[vpn].use = false;
    pageTable[vpn].dirty = false;
    pageTable[vpn].readOnly = false;

    frameTable[ppn].valid = true;
    frameTable[ppn].lock = false;
    frameTable[ppn].pageTable = pageTable;
    frameTable[ppn].vpn = vpn;
    frameTable[ppn].count = count++;
}

int MemoryManager::SavePage(){
    unsigned page = 3, value = UINT_MAX;
    for (size_t i = 0; i < NumPhysPages; i++) {
        ASSERT(frameTable[i].valid);
        if(frameTable[i].count <= value){
            value = frameTable[i].count;
            page = i;
        }
    }

    for(size_t i = 0; i < NumSectors; ++i){

        if(!swapTable[i].valid){
            kernel->synchDisk->WriteSector(i, kernel->machine->mainMemory + page * PageSize);
            DEBUG(dbgAddr, "save    vpn" << frameTable[page].vpn  << ", ppn" << page << ",   to segment" << i << ",");

            swapTable[i].valid = true;
            swapTable[i].lock = false;
            swapTable[i].pageTable =frameTable[page].pageTable;
            swapTable[i].vpn = frameTable[page].vpn;

            ASSERT(frameTable[page].pageTable[frameTable[page].vpn].valid);
            frameTable[page].valid = false;
            frameTable[page].pageTable[frameTable[page].vpn].valid = false;
            return page;
        }
    }

    return -1;
}

bool MemoryManager::RestorePage(TranslationEntry *pageTable, int vpn, int ppn){
    ASSERT(!frameTable[ppn].valid);
    ASSERT(!pageTable[vpn].valid);

    for (size_t i = 0; i < NumSectors; ++i){
        if(swapTable[i].valid && swapTable[i].pageTable == pageTable && swapTable[i].vpn == vpn){
            kernel->synchDisk->ReadSector(i, kernel->machine->mainMemory + (ppn * PageSize));
            DEBUG(dbgAddr, "restore vpn" << vpn  << ", ppn" << ppn << ",from segment" << i << ",");

            frameTable[ppn].valid = true;
            frameTable[ppn].lock = false;
            frameTable[ppn].pageTable = pageTable;
            frameTable[ppn].vpn = vpn;
            frameTable[ppn].count = count++;

            pageTable[vpn].valid = true;
            pageTable[vpn].dirty = false;
            pageTable[vpn].physicalPage = ppn;
            pageTable[vpn].readOnly = false;
            pageTable[vpn].use = true;
            pageTable[vpn].virtualPage = vpn;

            swapTable[i].valid = false;

            kernel->stats->numPageFaults++;
            return true;
        }
    }

    return false;
}