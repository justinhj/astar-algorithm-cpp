/*

A* Algorithm Implementation using STL is
Copyright (C)2001-2005 Justin Heyes-Jones

Permission is given by the author to freely redistribute and
include this code in any program as long as this credit is
given where due.

  COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
  INCLUDING, WITHOUT LIMITATION, WARRANTIES THAT THE COVERED CODE
  IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
  OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND
  PERFORMANCE OF THE COVERED CODE IS WITH YOU. SHOULD ANY COVERED
  CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE INITIAL
  DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY
  NECESSARY SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF
  WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE. NO USE
  OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
  THIS DISCLAIMER.

  Use at your own risk!



  FixedSizeAllocator class
  Copyright 2001 Justin Heyes-Jones

  This class is a constant time O(1) memory manager for objects of
  a specified type. The type is specified using a template class.

  Memory is allocated from a fixed size buffer which you can specify in the
  class constructor or use the default.

  Using GetFirst and GetNext it is possible to iterate through the elements
  one by one, and this would be the most common use for the class.

  I would suggest using this class when you want O(1) add and delete
  and you don't do much searching, which would be O(n). Structures such as binary
  trees can be used instead to get O(logn) access time.

*/

#ifndef FSA_H
#define FSA_H

#include <assert.h>
#include <cstdint>
#include <stdio.h>
#include <string.h>

template <class USER_TYPE>
class FixedSizeAllocator {
   public:
    // Constants
    enum { FSA_DEFAULT_SIZE = 100 };

    // This class enables us to transparently manage the extra data
    // needed to enable the user class to form part of the double-linked
    // list class
    struct FSA_ELEMENT {
        USER_TYPE UserType;

        FSA_ELEMENT* pPrev;
        FSA_ELEMENT* pNext;
        bool bAllocated;
    };

   public:  // methods
    FixedSizeAllocator(unsigned int MaxElements = FSA_DEFAULT_SIZE)
        : m_pFirstFree(nullptr),
          m_pFirstUsed(nullptr),
          m_MaxElements(MaxElements),
          m_pMemory(nullptr) {
        // Allocate enough memory for the maximum number of elements

        char* pMem = new char[m_MaxElements * sizeof(FSA_ELEMENT)];

        m_pMemory = (FSA_ELEMENT*)pMem;

        // Set the free list first pointer
        m_pFirstFree = m_pMemory;

        // Clear the memory
        memset(m_pMemory, 0, sizeof(FSA_ELEMENT) * m_MaxElements);

        // Point at first element
        FSA_ELEMENT* pElement = m_pFirstFree;

        // Set the double linked free list
        for (unsigned int i = 0; i < m_MaxElements; i++) {
            pElement->pPrev = pElement - 1;
            pElement->pNext = pElement + 1;
            pElement->bAllocated = false;

            pElement++;
        }

        // first element should have a null prev
        m_pFirstFree->pPrev = nullptr;
        // last element should have a null next
        (pElement - 1)->pNext = nullptr;
    }

    ~FixedSizeAllocator() {
        // Destroy any live objects remaining on the used list
        FSA_ELEMENT* pNode = m_pFirstUsed;
        while (pNode) {
            FSA_ELEMENT* pNext = pNode->pNext;
            pNode->UserType.~USER_TYPE();
            pNode->bAllocated = false;
            pNode = pNext;
        }
        m_pFirstUsed = nullptr;

        // Free up the memory
        delete[] (char*)m_pMemory;
        m_pMemory = nullptr;
        m_pFirstFree = nullptr;
    }

    // Allocate a new USER_TYPE and return a pointer to it
    USER_TYPE* alloc() {
        FSA_ELEMENT* pNewNode = nullptr;

        if (!m_pFirstFree) {
            return nullptr;
        } else {
            pNewNode = m_pFirstFree;
            m_pFirstFree = pNewNode->pNext;

            // if the new node points to another free node then
            // change that nodes prev free pointer...
            if (pNewNode->pNext) {
                pNewNode->pNext->pPrev = nullptr;
            }

            // node is now on the used list

            pNewNode->pPrev = nullptr;  // the allocated node is always first in the list

            if (m_pFirstUsed == nullptr) {
                pNewNode->pNext = nullptr;  // no other nodes
            } else {
                m_pFirstUsed->pPrev = pNewNode;  // insert this at the head of the used list
                pNewNode->pNext = m_pFirstUsed;
            }

            m_pFirstUsed = pNewNode;
            pNewNode->bAllocated = true;
        }

        return reinterpret_cast<USER_TYPE*>(pNewNode);
    }

    // Free the given user type
    // Guarded against invalid pointer, out of bounds, and double-free
    void free(USER_TYPE* user_data) {
        if (!user_data) {
            return;
        }

        FSA_ELEMENT* pNode = reinterpret_cast<FSA_ELEMENT*>(user_data);

        // Verify the pointer was allocated from this allocator
        assert(pNode >= m_pMemory && pNode < m_pMemory + m_MaxElements);
        assert(((uintptr_t)((char*)pNode - (char*)m_pMemory) % sizeof(FSA_ELEMENT)) == 0);
        if (pNode < m_pMemory || pNode >= m_pMemory + m_MaxElements ||
            ((uintptr_t)((char*)pNode - (char*)m_pMemory) % sizeof(FSA_ELEMENT)) != 0) {
            return;
        }

        // Guard against double-free
        assert(pNode->bAllocated);
        if (!pNode->bAllocated) {
            return;
        }
        pNode->bAllocated = false;

        // manage used list, remove this node from it
        if (pNode->pPrev) {
            pNode->pPrev->pNext = pNode->pNext;
        } else {
            // this handles the case that we delete the first node in the used list
            m_pFirstUsed = pNode->pNext;
        }

        if (pNode->pNext) {
            pNode->pNext->pPrev = pNode->pPrev;
        }

        // add to free list
        if (m_pFirstFree == nullptr) {
            // free list was empty
            m_pFirstFree = pNode;
            pNode->pPrev = nullptr;
            pNode->pNext = nullptr;
        } else {
            // Add this node at the start of the free list
            m_pFirstFree->pPrev = pNode;
            pNode->pNext = m_pFirstFree;
            m_pFirstFree = pNode;
        }
    }

    // For debugging this displays both lists (using the prev/next list pointers)
    void Debug() {
        printf("free list ");

        FSA_ELEMENT* p = m_pFirstFree;
        while (p) {
            printf("%p!%p ", (void*)p->pPrev, (void*)p->pNext);
            p = p->pNext;
        }
        printf("\n");

        printf("used list ");

        p = m_pFirstUsed;
        while (p) {
            printf("%p!%p ", (void*)p->pPrev, (void*)p->pNext);
            p = p->pNext;
        }
        printf("\n");
    }

    // Iterators

    USER_TYPE* GetFirst() {
        return reinterpret_cast<USER_TYPE*>(m_pFirstUsed);
    }

    USER_TYPE* GetNext(USER_TYPE* node) {
        if (!node) {
            return nullptr;
        }
        return reinterpret_cast<USER_TYPE*>((reinterpret_cast<FSA_ELEMENT*>(node))->pNext);
    }

   public:   // data
   private:  // methods
   private:  // data
    FSA_ELEMENT* m_pFirstFree;
    FSA_ELEMENT* m_pFirstUsed;
    unsigned int m_MaxElements;
    FSA_ELEMENT* m_pMemory;
};

#endif  // defined FSA_H
