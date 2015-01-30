//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

// ==++==
// 

//

// 
// ==--==


#ifndef _SIMPLERHASHTABLE_H_
#define _SIMPLERHASHTABLE_H_

// #include "utilcode.h" // for string hash functions
// #include "clrtypes.h"
// #include "check.h"
// #include "iterator.h"
#include "iallocator.h"

// SimplerHashTable implements a mapping from a Key type to a Value type,
// via a hash table.

// Synchronization is the responsibility of the caller: if a
// SimplerHash is used in a multithreaded context, the table should be
// associated with a lock.

// SimplerHashTable actually takes four template arguments: Key,
// KeyFuncs, Value, and Behavior.  We don't assume that Key has hash or equality
// functions specific names; rather, we assume that KeyFuncs has
// statics methods
//    int GetHashCode(Key)
// and
//    bool Equals(Key, Key)
// and use those.  An
// instantiator can thus make a small "adaptor class" to invoke
// existing instance method hash and/or equality functions.  If the
// implementor of a candidate Key class K understands this convention,
// these static methods can be implemented by K, so that K can be used
// as the actual arguments for the both Key and KeyTrait classes.
// 
// The "Behavior" argument provides the following static members:
//
// static const bool s_NoThrow                  true if GetKey, Equals, and hash are NOTHROW functions.  
//                                              Affects the THROWS clauses of several SHash functions.
//                                              (Note that the Null- and Deleted-related functions below 
//                                              are not affected by this and must always be NOTHROW.)
//
// s_growth_factor_numerator
// s_growth_factor_denominator                  Factor to grow allocation (numerator/denominator).  
//                                              Typically inherited from default traits (3/2)
//
// s_density_factor_numerator                   
// s_density_factor_denominator                 Maxium occupied density of table before growth 
//                                              occurs (num/denom).  Typically inherited (3/4).
//
// s_minimum_allocation                         Minimum table allocation count (size on first growth.)  It is 
//                                              probably preferable to call Reallocate on initialization rather
//                                              than override his from the default traits. (7)
//



class DefaultSimplerHashBehavior
{
  public:
    static const unsigned s_growth_factor_numerator = 3;
    static const unsigned s_growth_factor_denominator = 2;

    static const unsigned s_density_factor_numerator = 3;
    static const unsigned s_density_factor_denominator = 4;

    static const unsigned s_minimum_allocation = 7;

    static const bool s_NoThrow = true;
};


// stores info about primes, including the magic number and shift amount needed
// to implement a divide without using the divide instruction

class PrimeInfo
{
public:
    PrimeInfo() : prime(0), magic(0), shift(0) {}
    PrimeInfo(unsigned p, unsigned m, unsigned s) : prime(p), magic(m), shift(s) {}
    unsigned prime;
    unsigned magic;
    unsigned shift;
};


// Hash table class definition

template <typename Key, typename KeyFuncs, typename Value, typename Behavior>
class SimplerHashTable
{
  public:

    // Forward declaration.
    class KeyIterator;

    // Constructor/destructor.  SHash tables always start out empty, with no
    // allocation overhead.  Call Reallocate to prime with an initial size if
    // desired. Pass NULL as the IAllocator* if you want to use DefaultAllocator
    // (basically, operator new/delete).

    SimplerHashTable(IAllocator* alloc);
    ~SimplerHashTable();

    // We wish to provide overloaded operator new/deletes that take an
    // "IAllocator", so that an shash may be dynamically allocated
    // using a non-standard allocator.  If we do so, and in some
    // contexts still want to use the default operator new, C++ also requires us
    // to declare overloads for the standard new and delete operators.

    void * operator new(size_t sz)
    {
        CONTRACTL
        {
            THROWS;
            GC_NOTRIGGER;
        }
        CONTRACTL_END;
        return ::operator new(sz);
    }

    void * operator new(size_t sz, const NoThrow& tothrow)
    {
        CONTRACTL
        {
            if (nothrow.x== tothrow.x)NOTHROW; else THROWS;
            GC_NOTRIGGER;
        }
        CONTRACTL_END;
        return ::operator new(sz, tothrow);
    }

    void   operator delete(void* p)
    {
        CONTRACTL
        {
            NOTHROW;
            GC_NOTRIGGER;
        }
        CONTRACTL_END;
        ::operator delete(p);
    }

    void   operator delete[](void* p)
    {
        CONTRACTL
        {
            NOTHROW;
            GC_NOTRIGGER;
        }
        CONTRACTL_END;
        ::operator delete[](p);
    }

    // operators new/delete when an IAllocator is to be used.
    void * operator new(size_t sz, IAllocator * alloc);
    void * operator new[](size_t sz, IAllocator * alloc);
    void   operator delete(void * p, IAllocator * alloc);
    void   operator delete[](void * p, IAllocator * alloc);

    // If the table contains a mapping for "key", returns "true" and
    // sets "*pVal" to the value to which "key" maps.  Otherwise,
    // returns false, and does not modify "*pVal".
    bool Lookup(Key k, Value* pVal = NULL) const;

    Value *LookupPointer(Key k) const;

    unsigned GetIndexForKey(Key k) const;

    // Causes the table to map "key" to "val".  Returns "true" if
    // "key" had already been mapped by the table, "false" otherwise.
    bool Set(Key k, Value val);

    // Ensures that "key" is not mapped to a value by the "table."
    // Returns "true" iff it had been mapped.
    bool Remove(Key k);

    // Remove the mapping to which the current iterator "points"
    // (i.e., will yield).
    void Remove(KeyIterator& i);

    // Remove all mappings in the table.
    void RemoveAll();

    // Begin and End pointers for iteration over entire table. 
    KeyIterator Begin() const;
    KeyIterator End() const;

    // Return the number of elements currently stored in the table
    unsigned GetCount() const; 

  private:

    // Forward declaration of the linked-list node class.
    class Node;

    // If the table has a mapping for "k", return the node containing
    // that mapping, else "NULL".
    Node* FindNode(Key k) const;

    // Resizes a hash table for growth.  The new size is computed based
    // on the current population, growth factor, and maximum density factor.
    void Grow();

    // See if it is OK to grow the hash table by one element.  If not, reallocate
    // the hash table.
    void CheckGrowth();

  public:

    // Reallocates a hash table to a specific size.  The size must be big enough
    // to hold all elements in the table appropriately.  
    //
    // Note that the actual table size must always be a prime number; the number
    // passed in will be upward adjusted if necessary.
    void Reallocate(unsigned newTableSize);

    // For iteration, we use a pattern similar to the STL "forward
    // iterator" pattern.  It basically consists of wrapping an
    // "iteration variable" in an object, and providing pointer-like
    // operators on the iterator. Example usage:
    //
    // for (SimplerHashTable::KeyIterator iter = foo->Begin(), end = foo->End(); !iter.Equal(end); iter++)
    // {
    //      // use foo, iter.
    // }
    // iter.Get() will yield (a reference to) the
    // current key.  It will assert the equivalent of "iter != end."
    //

    class KeyIterator
    {
        friend class SimplerHashTable;

        // The method implementations have to be here for portability.
        // Some compilers won't compile the separate implementation in shash.inl

        Node**      m_table;
        Node*       m_node;
        unsigned    m_tableSize;
        unsigned    m_index;

      public:

        KeyIterator(const SimplerHashTable *hash, BOOL begin)
        : m_table(hash->m_table),
          m_node(NULL),
          m_tableSize(hash->m_tableSizeInfo.prime),
          m_index(begin ? 0 : m_tableSize)
        {
            LIMITED_METHOD_CONTRACT;
            if (begin && hash->m_tableCount > 0)
            {
                assert(m_table != NULL);
                while (m_index < m_tableSize && m_table[m_index] == NULL)
                    m_index++;

                if (m_index >= m_tableSize)
                {
                    return;
                }
                else
                {
                    m_node = m_table[m_index];
                }
                assert(m_node != NULL);
            }
        }

        const Key& Get() const
        {
            LIMITED_METHOD_CONTRACT;

            assert(m_node != NULL);

            return m_node->m_key;
        }

        const Value& GetValue() const
        {
            LIMITED_METHOD_CONTRACT;

            assert(m_node != NULL);

            return m_node->m_val;
        }

        void SetValue(const Value & value) const
        {
            LIMITED_METHOD_CONTRACT;

            assert(m_node != NULL);

            m_node->m_val = value;
        }

        void Next()
        {
            LIMITED_METHOD_CONTRACT;

            if (m_node != NULL)
            {
                m_node = m_node->m_next;
                if (m_node != NULL)
                {
                    return;
                }

                // Otherwise...
                m_index++;
            }
            while (m_index < m_tableSize && m_table[m_index] == NULL)
                m_index++;

            if (m_index >= m_tableSize)
            {
                m_node = NULL;
                return;
            }
            else
            {
                m_node = m_table[m_index];
            }
            assert(m_node != NULL);
        }

        bool Equal(const KeyIterator &i) const
        { 
            LIMITED_METHOD_CONTRACT;
            return i.m_node == m_node; 
        }

        CHECK DoCheck() const
        {
            CHECK_OK;
        }

        void operator++() {
            Next();
        }
        void operator++(int) {
            Next();
        }
    };

    // HashTableRef only exists to support operator[]
    // operator[] returns a HashTableRef which enables operator[] to support reading and writing
    // in a normal array, it just returns a ref an actual element, which is not possible here.
    class HashTableRef
    {
    public:
        // this is really the getter for the array.
        operator Value() 
        {
            
            Value result;
            table->Lookup(key, &result);
            return result;
        }

        void operator =(const Value v)
        {
            table->Set(key, v);
        }

        friend class SimplerHashTable;

    protected:
        HashTableRef(SimplerHashTable *t, Key k) 
        { 
            table = t;
            key = k;
        }

        SimplerHashTable *table;
        Key key;
    };

    Value &operator[](Key k) const
    {
        Value* p = LookupPointer(k);
        assert(p);
        return *p;
    }

  private:

    // Find the next prime number >= the given value.  

    static PrimeInfo NextPrime(unsigned number);

    // Instance members

    IAllocator*   m_alloc;                // IAllocator to use in this
                                          // table.
    // The node type.
    struct Node {
        Node* m_next;   // Assume that the alignment requirement of Key and Value are no greater than Node*, so put m_next to avoid unnecessary padding.
        Key   m_key;
        Value m_val;

        Node(Key k, Value v, Node* next): m_key(k), m_val(v), m_next(next) {}

        void* operator new(size_t sz, IAllocator* alloc)
        {
            return alloc->Alloc(sz);
        }

        void operator delete(void* p, IAllocator* alloc)
        {
            alloc->Free(p);
        }
    };

    Node**        m_table;                // pointer to table
    PrimeInfo     m_tableSizeInfo;        // size of table (a prime) and information about it
    unsigned      m_tableCount;           // number of elements in table
    unsigned      m_tableMax;             // maximum occupied count
};

#include "simplerhash.inl"

// A few simple KeyFuncs types...

// Base class for types whose equality function is the same as their "==".
template<typename T>
struct KeyFuncsDefEquals
{
    static bool Equals(const T& x, const T& y)
    {
        return x == y;
    }
};

template<typename T>
struct PtrKeyFuncs: public KeyFuncsDefEquals<const T*>
{
public:
    static unsigned GetHashCode(const T* ptr)
    {
        return (unsigned)ptr;  // Hmm.  Maybe (unsigned) ought to be "ssize_t" -- or this ought to be ifdef'd by size.
    }
};



template<typename T> // Must be coercable to "unsigned" with no loss of information.
struct SmallPrimitiveKeyFuncs: public KeyFuncsDefEquals<T>
{
    static unsigned GetHashCode(const T& val)
    {
        return static_cast<unsigned>(val);
    }
};

template<typename T> // Assumed to be of size sizeof(UINT64).
struct LargePrimitiveKeyFuncs: public KeyFuncsDefEquals<T>
{
    static unsigned GetHashCode(const T val)
    {
        UINT64 asUINT64 = static_cast<UINT64>(val);
        unsigned res = asUINT64 >> 32;
        res = res ^ static_cast<unsigned>(asUINT64 & ((((UINT64)1) << 32) - 1));
        return res;
    }
};

#endif // _SIMPLERHASHTABLE_H_
