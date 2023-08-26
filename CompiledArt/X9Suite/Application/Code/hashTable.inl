                                                     

template <typename t_key, typename t_data>
bool Enumerator<t_key, t_data>::EnumNext( void )
{
   return pHash->EnumNext( this );
}

template <typename t_key, typename t_data>
HashTable<t_key, t_data>::HashTable( 
   size_t startItems, //= 16
   size_t growBy, //= 16
   HashFunction pHash, //= StringHash
   CompareFunction pCompare //= StringCompare
) : m_ItemPool( startItems )
{
   m_GrowBy = growBy;
   m_NumItems = startItems;                                                     
   
   m_pHashFunction = pHash;
   m_pCompareFunction = pCompare;

   memset( m_Buckets, 0, sizeof(m_Buckets) );
}

template <typename t_key, typename t_data>
HashTable<t_key, t_data>::~HashTable( void )
{
   Clear( );
}

template <typename t_key, typename t_data>
void HashTable<t_key, t_data>::Add(
   const t_key &key,
   const t_data &data
)
{
   uint32 hashKey, hash;

   hash = m_pHashFunction( key );
   hashKey = hash % BucketCount;
   
   Item *pPrevItem = NULL;
   Item *pItem = m_Buckets[ hashKey ].pItems;

   //go through all the bucket items
   //and see if we have one which is already using our key
   while ( pItem )
   {
      if ( m_pCompareFunction(pItem->key, key) )
      {
         _ASSERTE( FALSE && "Key Value Pair already added to hashtable" );
         break;
      }
      
      //save previous item so we can link
      //if we need to create a new item
      pPrevItem = pItem;
      pItem = pItem->pNextInBucket;
   }
   
   //no matching keys, create a new item
   //and link with the previous
   if ( NULL == pItem )
   {
      pItem = GetFreeItem( );
      pItem->pNextInBucket = NULL;
      
      //first in the bucket
      if ( NULL == pPrevItem )
      {
         m_Buckets[ hashKey ].pItems = pItem;
      }
      else
      {  
         //link with the old in the bucket
         pPrevItem->pNextInBucket = pItem;
      }
   }
   
   //whether a new item was created
   //or we are using one w/ the matching key
   //copy our new data over
   memcpy( &pItem->data, &data, sizeof(t_data) );
   memcpy( &pItem->key,  &key,  sizeof(t_key) );
}

template <typename t_key, typename t_data>
void HashTable<t_key, t_data>::Remove(
   const t_key &key
)
{
   uint32 hash, hashKey;

   hash = m_pHashFunction( key );
   hashKey = hash % BucketCount;

   Item *pPrevBucketItem = NULL;
   Item *pBucketItem     = m_Buckets[ hashKey ].pItems;

   while ( pBucketItem )
   {
      if ( m_pCompareFunction(pBucketItem->key, key) )
      {  
         //restore link with previous bucket item
         if ( pPrevBucketItem )
         {
            pPrevBucketItem->pNextInBucket = pBucketItem->pNextInBucket;
         }
         else
         {
            //if there wasn't a previous bucket item
            //then restore the forward link from the head
            m_Buckets[ hashKey ].pItems = pBucketItem->pNextInBucket;
         }

         FreeItem( pBucketItem );
         return;
      }

      pPrevBucketItem = pBucketItem;
      pBucketItem     = pBucketItem->pNextInBucket;
   }
}

template <typename t_key, typename t_data>
bool HashTable<t_key, t_data>::Remove(
   const t_key &key,
   t_key *pKey,
   t_data *pData
)
{
   t_key *pPointerKey;
   t_data *pPointerData;
   
   if ( GetPointer(key, &pPointerKey, &pPointerData) )
   {
      memcpy( pKey, pPointerKey, sizeof(t_key) );
      memcpy( pData, pPointerData, sizeof(t_data) );

      Remove( key );
      return true;
   }
   
   return false;
}

template <typename t_key, typename t_data>
void HashTable<t_key, t_data>::Copy(
   const HashTable<t_key, t_data> &copyFrom
)
{
   Clear( );

   Enumerator<t_key, t_data> e = copyFrom.GetEnumerator( );
   while ( e.EnumNext( ) )
   {
      Add( e.Key( ), e.Data( ) );
   }
}
   
template <typename t_key, typename t_data>
bool HashTable<t_key, t_data>::Get(
   const t_key &key,
   t_data *pData
) const
{
   t_key *pPointerKey;
   t_data *pPointerData;
   
   if ( GetPointer(key, &pPointerKey, &pPointerData) )
   {
      memcpy( pData, pPointerData, sizeof(t_data) );
      return true;
   }
   
   return false;
}

template <typename t_key, typename t_data>
bool HashTable<t_key, t_data>::Contains(
   const t_key &key
) const
{
   uint32 hash, hashKey;

   hash = m_pHashFunction( key );
   hashKey  = hash % BucketCount;

   Item *pBucketItem = m_Buckets[ hashKey ].pItems;

   while ( pBucketItem )
   {
      if ( m_pCompareFunction(pBucketItem->key, key) )
      {  
         return true;
      }

      pBucketItem = pBucketItem->pNextInBucket;
   }

   return false;
}

template <typename t_key, typename t_data>
void HashTable<t_key, t_data>::Clear( void )
{
   m_ItemPool.Reset( );

   memset( m_Buckets, 0, sizeof(m_Buckets) );
}

template <typename t_key, typename t_data>
bool HashTable<t_key, t_data>::GetPointer(
   const t_key &key,
   t_key **pKey,
   t_data **pData
) const
{
   uint32 hash, hashKey;

   hash = m_pHashFunction( key );
   hashKey  = hash % BucketCount;

   Item *pBucketItem = m_Buckets[ hashKey ].pItems;

   while ( pBucketItem )
   {
      if ( m_pCompareFunction(pBucketItem->key, key) )
      {  
         (*pData) = &pBucketItem->data;
         (*pKey)  = &pBucketItem->key;
         return true;
      }

      pBucketItem = pBucketItem->pNextInBucket;
   }

   return false;
}

template <typename t_key, typename t_data>
bool HashTable<t_key, t_data>::EnumNext(
   Enumerator<t_key, t_data> *pEnumerator
) const
{
   if ( NULL == pEnumerator->pItem )
   {
      uint32 i;

      for ( i = pEnumerator->bucket + 1; i < BucketCount; i++ )
      {
         if ( m_Buckets[ i ].pItems )
         {
            pEnumerator->pItem = m_Buckets[ i ].pItems;
            pEnumerator->bucket = i;
            break;
         }
      }

      if ( i == BucketCount ) return false;
   }
   else
   {
      pEnumerator->pItem = ((Item *)pEnumerator->pItem)->pNextInBucket;
      
      if ( NULL == pEnumerator->pItem )
      {
         return EnumNext( pEnumerator );
      }
   }

   pEnumerator->pKey  = &((Item *)pEnumerator->pItem)->key;
   pEnumerator->pData = &((Item *)pEnumerator->pItem)->data;

   return true;
}

template <typename t_key, typename t_data>
typename HashTable<t_key, t_data>::Item *HashTable<t_key, t_data>::GetFreeItem( void )
{
   return m_ItemPool.Alloc( );
}

template <typename t_key, typename t_data>
void HashTable<t_key, t_data>::FreeItem(
   Item *pItem
)
{
   m_ItemPool.Free( pItem );
}

uint32 HashFunctions::StringHash( 
   const char *pString
)
{
   //hash routine found here
   //http://www.azillionmonkeys.com/qed/hash.html

   uint32 hash, temp;
   const char *pData;
   int remainder;

   int length = (int) strlen( pString );

   hash  = length;
   pData = pString;

   remainder = length % 4;
   length >>= 2;

   while ( length > 0 )
   {
      hash  += *(uint16 *)pData;
      temp   = ( *(uint16 *)(pData + 2) << 11 ) ^ hash;
      hash   = ( hash << 16 ) ^ temp;
      pData += 2 * sizeof (uint16);
      hash  += hash >> 11;
      
      --length;
   }

   switch ( remainder ) 
   {
      case 3: 
         hash += *(uint16 *)pData;
         hash ^= hash << 16;
         hash ^= pData[ sizeof (uint16) ] << 18;
         hash += hash >> 11;
         break;

      case 2: 
         hash += *(uint16 *) pData;
         hash ^= hash << 11;
         hash += hash >> 17;
         break;
      
      case 1: 
         hash += *pData;
         hash ^= hash << 10;
         hash += hash >> 1;
   }

   hash ^= hash << 3;
   hash += hash >> 5;
   hash ^= hash << 4;
   hash += hash >> 17;
   hash ^= hash << 25;
   hash += hash >> 6;

   return hash;
}

bool HashFunctions::StringCompare( 
   const char *pString1,
   const char *pString2
)
{
   return 0 == strcmp(pString1, pString2);
}

uint32 HashFunctions::NUIntHash( 
   nuint value
)
{
   return (uint32) value;
}

bool HashFunctions::NUIntCompare( 
   nuint value1,
   nuint value2
)
{
   return value1 == value2;
}
