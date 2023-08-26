#pragma once

template <typename t_item>
class MemoryPool
{
public:
   struct Block
   {
      t_item *pItems;
   };

protected:
   size_t  m_ItemsInBlock;
   size_t  m_NumBlocks;
   size_t  m_NextFreeItem;

   Block   *m_pBlocks;
   t_item **m_pItemPointers;

public:
   MemoryPool(
      size_t numItems = 16
   );

   ~MemoryPool( void );

   void Reset( void );

   t_item *Alloc( void );

   void Free( t_item *pItem );
};

#include "MemoryPool.inl"
