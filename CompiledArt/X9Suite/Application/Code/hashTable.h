#pragma once

#include "Types.h"
#include "MemoryPool.h"

class HashFunctions
{
public:
   static inline uint32 StringHash( 
      const char *pString
   );

   static inline bool StringCompare(
      const char *pString1,
      const char *pString2
   );

   static inline uint32 NUIntHash( 
      nuint value
   );

   static inline bool NUIntCompare(
      nuint value1,
      nuint value2
   );
};

template <typename t_key, typename t_data>
class HashTable;

template <typename t_key, typename t_data>
class Enumerator
{
   friend class HashTable<t_key, t_data>;

public:
   Enumerator(const HashTable<t_key, t_data> *_pHash)
   {
      pHash  = _pHash;
      bucket = -1;
      pItem  = NULL;
   }
   
   bool EnumNext( void );

   const t_key  &Key ( void ) { return *pKey; }
   const t_data &Data( void ) { return *pData; }

private:
   Enumerator( ) {}

private:
   const HashTable<t_key, t_data> *pHash;

   uint32  bucket;
   void   *pItem;
   t_key  *pKey;
   t_data *pData;
};

template <typename t_key, typename t_data>
class HashTable
{
   friend class Enumerator<t_key, t_data>;

public:
   typedef bool   (*CompareFunction) (t_key, t_key);
   typedef uint32 (*HashFunction)    (t_key);

public:
   struct Item
   {
      t_key key;
      t_data data;

      Item *pNextInBucket;
   };

protected:
   struct Bucket
   {
      Item *pItems;
   };

   static const uint32 BucketCount = 1024;

protected:
   MemoryPool<Item> m_ItemPool;

   size_t  m_NumItems;
   size_t  m_GrowBy;
   Bucket  m_Buckets[ BucketCount ];

   Item   *m_pCurrentItem;
   uint32  m_CurrentBucket;

   CompareFunction m_pCompareFunction;
   HashFunction    m_pHashFunction;

public:
   HashTable(
      size_t startItems = 16,
      size_t growBy = 16,
      HashFunction pHash = HashFunctions::StringHash,
      CompareFunction pCompare = HashFunctions::StringCompare
   );

   ~HashTable( void );

   void Add(
      const t_key &key,
      const t_data &data
   );

   void Remove(
      const t_key &key
   );

   bool Remove(
      const t_key &key,
      t_key *pKey,
      t_data *pData
   );

   void Copy(
      const HashTable<t_key, t_data> &copyFrom
   );

   bool Get(
      const t_key &key,
      t_data *pData
   ) const;

   bool Contains(
      const t_key &key
   ) const;

   Enumerator<t_key, t_data> GetEnumerator( void ) const
   {
      return Enumerator<t_key, t_data>(this);
   }

   void Clear( void );

private:
   bool GetPointer(
      const t_key &key,
      t_key **pKey,
      t_data **pData
   ) const;

   bool EnumNext(
      Enumerator<t_key, t_data> *pEnumerator
   ) const;

   Item *GetFreeItem( void );
   
   void FreeItem(
      Item *pItem
   );
};

#include "HashTable.inl"
