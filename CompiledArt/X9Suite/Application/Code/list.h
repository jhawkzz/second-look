#pragma once

#include "Types.h"

template <typename t_item>
class List
{
public:
   static const uint32 InvalidIndex = 0xffffffff;

protected:
   uint32 m_NumItems;
   uint32 m_GrowBy;
   uint32 m_MaxItems;

   t_item *m_pItems;

public:
   List(
      uint32 startItems = 16,
      uint32 growBy = 16
   );

   ~List( void );

   void Copy(
      const List<t_item> &
   );

   int Add(
      const t_item &item
   );

   void Insert(
      const t_item &item,
      uint32 indexPosition
   );

   int AddUnique(
      const t_item &item
   );
   
   void Remove(
      const t_item &item
   );

   void Remove(
      uint32 index
   );

   void RemoveSorted(
      const t_item &item
   );

   void RemoveSorted(
      uint32 index
   );

   t_item Get(
      uint32 index
   ) const;

   t_item *GetPointer(
      uint32 index
   ) const;

   uint32 GetIndex(
      t_item item
   ) const;
   
   List<t_item> & operator = (const List<t_item> &rhs);

   bool Contains(
      t_item item
   ) const
   {
      return InvalidIndex != GetIndex( item );
   }

   void Clear( void ) { m_NumItems = 0; }

   uint32 GetSize( void ) const { return m_NumItems; }
};

#include "List.inl"
