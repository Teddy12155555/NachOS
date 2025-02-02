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
#include "machine.h"
#include "noff.h"
#define ODDD_DEBUG 0

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

AddrSpace::AddrSpace()
{
    /*
    pageTable = new TranslationEntry[NumPhysPages];
    for (unsigned int i = 0; i < NumPhysPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virt page # = phys page #
	pageTable[i].physicalPage = i;
//	pageTable[i].physicalPage = 0;
	pageTable[i].valid = TRUE;
//	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  
    }
    */
    
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
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
unsigned char AddrSpace::PhysPageStatus[NumPhysPages] = {0};
unsigned int AddrSpace::FreePhysPages = NumPhysPages;

bool 
AddrSpace::Load(char *fileName) 
{
    // cout << "Enter AddrSpace::Load" << endl;
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

    cout << "============================" << endl;
    cout << "fileName:  " << fileName << endl;
    cout << "size "<< size << endl;
    cout << "codeSize: " << noffH.code.size << endl;
    cout << "init data Size: " << noffH.initData.size<< endl;
    cout << "uninitData.size: " << noffH.uninitData.size << endl;
    cout << "numPages:  " << numPages << endl;
    cout << "============================" << endl << endl;
//	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;
    size = numPages * PageSize;

    //hw2 create pageTable
    pageTable = new TranslationEntry[numPages];

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

// hw4 region
    if (noffH.code.size > 0) {
            for(unsigned int j=0,i=0;i < numPages ;i++){

                 // find useful memory
                 j=0;
                 while(kernel->machine->usedPhysPage[j]!=FALSE&&j<NumPhysPages){j++;}
                 
                 // if we have a useful memory
                 if(j<NumPhysPages){  
                  // set our physical page to "is used"
                  kernel->machine->usedPhysPage[j] = TRUE;

                  // assign it to our main memory page table
                  kernel->machine->mainMemTable[j] = &pageTable[i];

                  // set our page status
                  pageTable[i].physicalPage = j;
                  pageTable[i].valid = TRUE;
                  pageTable[i].use = FALSE;
                  pageTable[i].dirty = FALSE;
                  pageTable[i].readOnly = FALSE;
                  pageTable[i].count++; //for LRU
                  executable->ReadAt(&(kernel->machine->mainMemory[j*PageSize]),PageSize, noffH.code.inFileAddr+(i*PageSize));  
                 }
                 // if we don't have a useful memory
                else{ 
                      // out temporary buffer for the page
                      char *buf = new char[PageSize];

                      // find a useful virtual memory
                      int k = 0;
                      while(kernel->machine->usedVirPage[k]!=FALSE){k++;}

                      // check if we find a useful virtual memory
                      if( k > NumPhysPages)
                      {
                        cout << "Virtual Memory out of range" << endl;
                        return AddressErrorException;
                      }
                      kernel->machine->usedVirPage[k]=true;

                      // set our virtaul page status
                      pageTable[i].virtualPage=k;
                      pageTable[i].valid = FALSE;
                      pageTable[i].use = FALSE;
                      pageTable[i].dirty = FALSE;
                      pageTable[i].readOnly = FALSE;
                      executable->ReadAt(buf, PageSize, noffH.code.inFileAddr+(i*PageSize));
                      // write it to our disk
                      kernel->vm_Disk->WriteSector(k, buf);
                    }
                }
    }

	if (noffH.initData.size > 0) {
        unsigned int start = noffH.initData.virtualAddr;
        unsigned int end = start + noffH.initData.size;
        unsigned int tableIndex = 0;
        unsigned int offset = 0;
        unsigned int inFileAddr = noffH.initData.inFileAddr;
        {
            unsigned int physAddr = GetPhysAddr(start,tableIndex,offset);
            if(offset!=0){
                unsigned int size = PageSize - offset;
                executable->ReadAt(
                    &(kernel->machine->mainMemory[physAddr]),
                    size,inFileAddr);
                start += size;
                inFileAddr += size;
            }
        }
        while(start + PageSize < end){
            executable->ReadAt(
                &(kernel->machine->mainMemory[GetPhysAddr(start,tableIndex,offset)]),
                PageSize,inFileAddr);
            start+= PageSize;
            inFileAddr += PageSize;
        }
        {
            if(start != end){
                executable->ReadAt(
                    &(kernel->machine->mainMemory[GetPhysAddr(start,tableIndex,offset)]),
                    end-start,inFileAddr);
            }
        }
    }
    delete executable;			// close file
    return TRUE;			// success
}

unsigned int AddrSpace::GetPhysAddr(unsigned int virtualAddr,unsigned int& tableIdx, unsigned int & offset){
    tableIdx = virtualAddr/PageSize;
    offset = virtualAddr%PageSize;
    return pageTable[tableIdx].physicalPage *PageSize + offset;
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
    // hw4
    pageTableLoaded=FALSE;

    if (!Load(fileName)) {
	   cout << "inside !Load(FileName)" << endl;
	 return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register
    // hw4
    pageTableLoaded=TRUE;
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

void AddrSpace::SaveState() 
{
        if(pageTableLoaded){
          pageTable=kernel->machine->pageTable;
          numPages=kernel->machine->pageTableSize;
        }
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
