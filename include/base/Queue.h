#ifndef BASE_QUEUE_H
#define BASE_QUEUE_H

#include <string.h>

#include <stdexcept>

namespace base {

   template<class T, bool canexpand = false>
   class Queue {

      public:
         class Iterator {

            friend class Queue<T,canexpand>;

            Queue<T,canexpand>* prnt;
            T* item;
            unsigned loop;

            Iterator(Queue<T,canexpand>* _prnt, T* _item, unsigned _loop) : prnt(_prnt), item(_item), loop(_loop) {}

            public:

            Iterator() : prnt(0), item(0) {}
            Iterator(const Iterator& src) : prnt(src.prnt), item(src.item), loop(src.loop) {}

            T* operator()() const { return item; }

            bool operator==(const Iterator& src) const { return (item == src.item) && (loop == src.loop); }
            bool operator!=(const Iterator& src) const { return (item != src.item) || (loop != src.loop); }

            Iterator& operator=(const Iterator& src) { prnt = src.prnt; item = src.item; return *this; }

            // prefix ++iter
            Iterator& operator++()
            {
               if (++item == prnt->fBorder) { item = prnt->fQueue; loop++; }
               return *this;
            }

            // postfix  iter++
            void operator++(int)
            {
               if (++item == prnt->fBorder) { item = prnt->fQueue; loop++; }
            }
         };

      protected:

         friend class Queue<T,canexpand>::Iterator;

         T*         fQueue;
         T*         fBorder; // maximum pointer value
         unsigned   fCapacity;
         unsigned   fSize;
         T*         fHead;
         T*         fTail;
         unsigned   fInitSize; //!< original size of the queue, restored then clear() method is called

      public:
         Queue() :
            fQueue(0),
            fBorder(0),
            fCapacity(0),
            fSize(0),
            fHead(0),
            fTail(0),
            fInitSize(0)
         {
         }

         Queue(unsigned capacity) :
            fQueue(0),
            fBorder(0),
            fCapacity(0),
            fSize(0),
            fHead(0),
            fTail(0),
            fInitSize(capacity)
         {
            Allocate(capacity);
         }

         virtual ~Queue()
         {
            delete[] fQueue;
            fQueue = 0;
         }

         void Init(unsigned capacity)
         {
            fInitSize = capacity;
            Allocate(capacity);
         }

         void Allocate(unsigned capacity)
         {
            if (fCapacity>0) {
               delete[] fQueue;
               fCapacity = 0;
               fQueue = 0;
               fBorder = 0;
            }

            if (capacity>0) {
              fCapacity = capacity;
              fSize = 0;
              fQueue = new T [fCapacity];
              fBorder = fQueue + fCapacity;
              fHead = fQueue;
              fTail = fQueue;
            }
         }

         /** \brief Method can be used to copy content of the
          * queue into externally allocated array */
         virtual void CopyTo(T* tgt)
         {
            if (fSize>0) {
               if (fHead>fTail) {
                  memcpy(tgt, fTail, (fHead - fTail) * sizeof(T));
               } else {
                  unsigned sz = fBorder - fTail;
                  memcpy(tgt, fTail, sz * sizeof(T));
                  memcpy(tgt + sz, fQueue, (fHead - fQueue) * sizeof(T));
               }
            }
         }

         // increase capacity of queue without lost of content
         bool Expand(unsigned newcapacity = 0)
         {
            if (!canexpand) return false;

            if (newcapacity <= fCapacity)
               newcapacity = fCapacity * 2;
            if (newcapacity < 16) newcapacity = 16;
            T* q = new T [newcapacity];
            if (q==0) return false;

            CopyTo(q);

            delete [] fQueue;

            fCapacity = newcapacity;
            fQueue = q;
            fBorder = fQueue + fCapacity;
            fHead = fQueue + fSize; // we are sure that size less than capacity
            fTail = fQueue;
            return true;
         }

         bool Remove(T value)
         {
            if (size()==0) return false;

            T *i_tgt(fTail), *i_src(fTail);

            do {
               if (*i_src != value) {
                  if (i_tgt!=i_src) *i_tgt = *i_src;
                  if (++i_tgt == fBorder) i_tgt = fQueue;
               } else
                  fSize--;
               if (++i_src == fBorder) i_src = fQueue;
            } while (i_src != fHead);

            fHead = i_tgt;

            return i_tgt != i_src;
         }

         bool erase_item(unsigned indx)
         {
            if (indx>=fSize) return false;

            T* tgt = fTail + indx;
            if (tgt>=fBorder) tgt -= fCapacity;

            while (true) {
               T* src = tgt + 1;
               if (src==fBorder) src = fQueue;
               if (src==fHead) break;
               *tgt = *src;
               tgt = src;
            }

            fHead = tgt;
            fSize--;
            return true;
         }

         bool MakePlaceForNext()
         {
            return (fSize<fCapacity) || Expand();
         }

         void push(const T& val)
         {
            if ((fSize<fCapacity) || Expand()) {
               *fHead++ = val;
               if (fHead==fBorder) fHead = fQueue;
               fSize++;
            } else {
               throw std::runtime_error("No more space in fixed-size queue");
            }
         }

         T* PushEmpty()
         {
            if ((fSize<fCapacity) || Expand()) {
               T* res = fHead;
               if (++fHead==fBorder) fHead = fQueue;
               fSize++;
               return res;
            }

            throw std::runtime_error("No more space in fixed queue size");
            return 0;
         }

         void pop()
         {
            if (fSize>0) {
               fSize--;
               if (++fTail==fBorder) fTail = fQueue;
            }
         }

         T pop_front()
         {
            #ifdef DABC_EXTRA_CHECKS
               if (fSize==0) EOUT(("Queue is empty"));
            #endif
            T* res = fTail++;
            if (fTail==fBorder) fTail = fQueue;
            fSize--;
            return *res;
         }

         T& front() const
         {
            #ifdef DABC_EXTRA_CHECKS
               if (fSize==0) EOUT(("Queue is empty"));
            #endif
            return *fTail;
         }

         // TODO: implement iterator to access all items in the queue

         T& item(unsigned indx) const
         {
            #ifdef DABC_EXTRA_CHECKS
            if (indx>=fSize)
               EOUT(("Wrong item index %u", indx));
            #endif
            T* item = fTail + indx;
            if (item>=fBorder) item -= fCapacity;
            return *item;
         }

         T* item_ptr(unsigned indx) const
         {
            #ifdef DABC_EXTRA_CHECKS
            if (indx>=fSize) {
               EOUT(("Wrong item index %u", indx));
               return 0;
            }
            #endif
            T* _item = fTail + indx;
            if (_item>=fBorder) _item -= fCapacity;
            return _item;
         }

         T& back() const
         {
            #ifdef DABC_EXTRA_CHECKS
               if (fSize==0) EOUT(("Queue is empty"));
            #endif
            return item(fSize-1);
         }

         void clear()
         {
            fHead = fQueue;
            fTail = fQueue;
            fSize = 0;
         }

         unsigned capacity() const { return fCapacity; }

         unsigned size() const { return fSize; }

         bool full() const { return capacity() == size(); }

         bool empty() const { return size() == 0; }

         Iterator begin() { return Iterator(this, fTail, 0); }
         Iterator end() { return Iterator(this, fHead, fHead > fTail ? 0 : 1); }
   };

   // ___________________________________________________________

   /** Special case of the queue when structure or class is used as entry of the queue.
    * Main difference from normal queue - one somehow should cleanup item when it is not longer used
    * without item destructor. The only way is to introduce reset() method in contained class which
    * do the job similar to destructor. It is also recommended that contained class has
    * copy constructor and assign operator defined. For example, one should have class:
    *   struct Rec {
    *      int i;
    *      bool b;
    *      Rec() : i(0), b(false) {}
    *      Rec(const Rec& src) : i(src.i), b(src.b) {}
    *      Rec& operator=(const Rec& src) { i = src.y; b = src.b; return *this; }
    *      void reset() { i = 0; b = false; }
    *      ~Rec() { reset(); }
    *   };
    *
    */

   template<class T, bool canexpand = true>
   class RecordsQueue : public Queue<T, canexpand> {
      typedef Queue<T, canexpand> Parent;
      public:
         RecordsQueue() : Parent() {}

         RecordsQueue(unsigned capacity) : Parent(capacity) {}

         T& front() const { return Parent::front(); }

         T& back() const { return Parent::back(); }

         unsigned capacity() const { return Parent::capacity(); }

         unsigned size() const { return Parent::size(); }

         bool full() const { return Parent::full(); }

         bool empty() const { return Parent::empty(); }

         T& item(unsigned indx) const { return Parent::item(indx); }

         T* item_ptr(unsigned indx) const { return Parent::item_ptr(indx); }

         void pop() { front().reset(); Parent::pop(); }

         T pop_front()
         {
            T res = front();
            pop();
            return res;
         }


         void pop_items(unsigned cnt)
         {
            if (cnt>=size()) {
               clear();
            } else {
               while (cnt-->0) pop();
            }
         }

         void clear()
         {
            if (Parent::fCapacity != Parent::fInitSize) {
               Parent::Allocate(Parent::fInitSize);
            } else {
               for (unsigned n=0;n<size();n++)
                  item(n).reset();
               Parent::clear();
            }
         }

         bool erase_item(unsigned indx)
         {
            bool res = Parent::erase_item(indx);

            // after item was removed from inside queue,
            // head element now pointer on item which was not cleared
            if (res) Parent::fHead->reset();

            return res;
         }


   };

}

#endif

