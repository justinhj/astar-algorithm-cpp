/*
  FixedSizeAllocator class
  Copyright 2001 Justin Heyes-Jones
  
  Simplified for O(1) allocation without used-list overhead.
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
    enum { FSA_DEFAULT_SIZE = 100 };

    struct FSA_ELEMENT {
        USER_TYPE UserType;
        FSA_ELEMENT* pNext;
    };

   public:
    FixedSizeAllocator(unsigned int MaxElements = FSA_DEFAULT_SIZE)
        : m_pFirstFree(nullptr),
          m_MaxElements(MaxElements),
          m_pMemory(nullptr) {
          
        char* pMem = new char[m_MaxElements * sizeof(FSA_ELEMENT)];
        m_pMemory = (FSA_ELEMENT*)pMem;
        m_pFirstFree = m_pMemory;

        FSA_ELEMENT* pElement = m_pFirstFree;
        for (unsigned int i = 0; i < m_MaxElements - 1; i++) {
            pElement->pNext = pElement + 1;
            pElement++;
        }
        pElement->pNext = nullptr;
    }

    ~FixedSizeAllocator() {
        if (m_pMemory) {
            delete[] (char*)m_pMemory;
            m_pMemory = nullptr;
            m_pFirstFree = nullptr;
        }
    }

    USER_TYPE* alloc() {
        if (!m_pFirstFree) {
            return nullptr;
        }

        FSA_ELEMENT* pNewNode = m_pFirstFree;
        m_pFirstFree = pNewNode->pNext;

        return reinterpret_cast<USER_TYPE*>(pNewNode);
    }

    void free(USER_TYPE* user_data) {
        if (!user_data) {
            return;
        }

        FSA_ELEMENT* pNode = reinterpret_cast<FSA_ELEMENT*>(user_data);

        // Guard against out of bounds (debug only as this is hot path)
        assert(pNode >= m_pMemory && pNode < m_pMemory + m_MaxElements);
        assert(((uintptr_t)((char*)pNode - (char*)m_pMemory) % sizeof(FSA_ELEMENT)) == 0);

        pNode->pNext = m_pFirstFree;
        m_pFirstFree = pNode;
    }

   private:
    FSA_ELEMENT* m_pFirstFree;
    unsigned int m_MaxElements;
    FSA_ELEMENT* m_pMemory;
};

#endif // defined FSA_H
