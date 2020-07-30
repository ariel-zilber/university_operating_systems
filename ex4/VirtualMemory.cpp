#include <algorithm>
#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

//--------------------------------------------------------- Typedefs-------------------------------------//

/** Return code on success **/
#define SUCCESS 1

/** Return code on failure **/
#define FAILURE 0

/** Frame not defined **/
#define FRAME_NOT_DEFINED (-1)

//--------------------------------------------------------- Definitions-----------------------------------------------//
void clearTable(uint64_t frameIndex);

/***
 * Return index of a given depth index
 * @param virtualAddress
 * @param depth the current page depth
 * @return
 */
uint64_t getPageNumber(uint64_t virtualAddress, int depth);

/***
 * Returns the address offset
 * @param virtualAddress the address of virtual memory
 * @return
 */
uint64_t getOffset(uint64_t virtualAddress);

/***
 *
 * @param frameIndex
 * @param evictedPageIndex
 * @param physicalAddress
 */
void evictFrame(uint64_t maxPage, uint64_t maxPageFrame, uint64_t maxParentAddress);

/***
 * Runs Depth-first search
 * @param addr
 * @param depth
 * @param parentAddr
 * @param pageSwappedIn
 * @param longestDistCyclic
 * @param pageNumber
 * @param emptyAddr
 * @param emptyAddrParent
 * @param maxIndex
 * @param maxPage
 * @param maxPageFrame
 * @param maxParentAddr
 * @param previousFrame
 */
void dfs(word_t addr, unsigned int currDepth, uint64_t parentAddr, int64_t pageSwappedIn, int &longestDistCyclic,
         unsigned long long pageNumber, uint64_t &emptyAddr, uint64_t &emptyAddrParent, word_t &maxIndex,
         unsigned long &maxPage, word_t &maxPageFrame, word_t &maxParentAddr, word_t &previousFrame
);

/***
 * Returns a free frame
 * @param virtualAddress
 * @param previousFrame previous free frame returned
 * @return
 */
uint64_t getFreeFrame(uint64_t virtualAddress, word_t previousFrame);

/***
 * Translates a given virtual address to physical one
 * @param virtualAddress the address to translate
 * @return the translated address
 */
uint64_t translateAddress(uint64_t virtualAddress);
//--------------------------------------------------------- Inner structs/classes-------------------------------------//



uint64_t getPageNumber(uint64_t virtualAddress, int depth)
{
    uint64_t address = virtualAddress >> OFFSET_WIDTH;
    address = address >> ((TABLES_DEPTH - (depth + 1)) * (OFFSET_WIDTH));
    return address & (PAGE_SIZE - 1);
}


uint64_t getOffset(uint64_t virtualAddress)
{
    return virtualAddress & (PAGE_SIZE - 1);
}


void evictFrame(uint64_t maxPage, uint64_t maxPageFrame, uint64_t maxParentAddress)
{
    PMevict(maxPageFrame, maxPage);
    PMwrite(maxParentAddress, 0);

}

void dfs(word_t addr, unsigned int currDepth, uint64_t parentAddr, int64_t pageSwappedIn, int &longestDistCyclic,
         unsigned long long pageNumber, uint64_t &emptyAddr, uint64_t &emptyAddrParent, word_t &maxIndex,
         unsigned long &maxPage, word_t &maxPageFrame, word_t &maxParentAddr, word_t &previousFrame
)
{
    int rowIndex;
    bool isEmpty = true;
    word_t childAddr;
    word_t currFrame;

    // update the current maximal index
    if (addr > maxIndex)
    {
        maxIndex = addr;
    }


    if (currDepth < TABLES_DEPTH)
    {
        // recurse on indexes:
        for (rowIndex = 0; rowIndex < PAGE_SIZE; rowIndex++)
        {
            PMread(addr * PAGE_SIZE + rowIndex, &childAddr);

            if (childAddr != 0)
            {
                isEmpty = false;
                dfs(childAddr,
                    currDepth + 1,
                    (addr * PAGE_SIZE + rowIndex),
                    pageSwappedIn,
                    longestDistCyclic,
                    (pageNumber << OFFSET_WIDTH) + rowIndex,
                    emptyAddr,
                    emptyAddrParent,
                    maxIndex,
                    maxPage,
                    maxPageFrame,
                    maxParentAddr,
                    previousFrame
                );
            }
        }

        //
        PMread(parentAddr, &currFrame);

        if (isEmpty && previousFrame != currFrame)
        {
            emptyAddr = addr;
            emptyAddrParent = parentAddr;
        }
    }
    else
    {
        // check cyclic
        long long distToPageNumber = std::abs((long) (pageSwappedIn - pageNumber));
        long long currCyclicDist = std::min(NUM_PAGES - distToPageNumber, distToPageNumber);
        if (currCyclicDist > longestDistCyclic)
        {
            longestDistCyclic = currCyclicDist;
            maxPage = pageNumber;
            PMread((uint64_t) parentAddr, &maxPageFrame);
            maxParentAddr = parentAddr;
        }
        return;
    }
}

uint64_t getFreeFrame(uint64_t virtualAddress, word_t previousFrame)
{
    uint64_t addrEmpty = 0;
    uint64_t addrEmptyParent = 0;
    word_t indexMax = 0;
    uint64_t maxPage = 0;
    word_t maxPageFrame = 0;
    word_t maxParentAddress = 0;
    int longestDistCyclic = 0;

    dfs(0, 0, 0, virtualAddress >> OFFSET_WIDTH,
        longestDistCyclic, 0, addrEmpty, addrEmptyParent, indexMax,
        maxPage, maxPageFrame, maxParentAddress, previousFrame
    );

    // choose a framework based on the priorities that were given in the exercise description
    if (addrEmpty != 0)
    {
        PMwrite(addrEmptyParent, 0);
        return addrEmpty;
    }

    if (indexMax + 1 < NUM_FRAMES)
    {
        return (indexMax + 1);
    }
    evictFrame(maxPage, maxPageFrame, maxParentAddress);
    return maxPageFrame;
}


uint64_t translateAddress(uint64_t virtualAddress)
{
    uint64_t offset = getOffset(virtualAddress);
    word_t addr = 0;
    uint64_t oldAddress = 0;
    uint64_t currFrame = 0;
    uint64_t previousFrame = FRAME_NOT_DEFINED;
    int currDepth = 0;

    while (currDepth < TABLES_DEPTH)
    {
        oldAddress = addr * PAGE_SIZE + getPageNumber(virtualAddress, currDepth);

        PMread(oldAddress, &addr);

        if (addr == 0)
        {
            currFrame = getFreeFrame(virtualAddress, previousFrame);
            clearTable(currFrame);
            PMwrite(oldAddress, currFrame);
            addr = currFrame;
        }

        // iterate to the next level
        previousFrame = addr;
        currDepth++;
    }


    if (currFrame != 0)
    {
        PMrestore(currFrame, virtualAddress >> OFFSET_WIDTH);
    }

    return (addr * PAGE_SIZE) + offset;


}

void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

//--------------------------------------------------------- Library functions  -----------------------------------------//



/*
 * Initialize the virtual memory
 */
void VMinitialize()
{
    clearTable(0);
}


/* reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t *value)
{

    if ((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) != 0)
        return FAILURE;

    uint64_t result = translateAddress(virtualAddress);
    PMread(result, value);

    return SUCCESS;
}


/* writes a word to the given virtual address
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value)
{
    if ((virtualAddress >> VIRTUAL_ADDRESS_WIDTH) != 0)
        return FAILURE;
    uint64_t result = translateAddress(virtualAddress);
    PMwrite(result, value);

    return SUCCESS;
}