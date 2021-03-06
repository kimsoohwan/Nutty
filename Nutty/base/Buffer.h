#pragma once
#include "Iterator.h"
#include "../Alloc.h"

namespace nutty
{
    namespace base
    {
        template<
            typename T,
            typename Content,
            typename Allocator = DefaultAllocator<T>
        >
        class Base_Buffer
        {
            friend class Iterator<T, Base_Buffer<T, Content, Allocator>>;
        private:
            __host__ Base_Buffer(const Base_Buffer& b) {}

            __host__ void _delete(void)
            {
                if(m_ptr)
                {
                    m_alloc.Deallocate(m_ptr);
                }
                m_size = 0;
                m_ptr = NULL;
            }

        public:
            __host__ Base_Buffer(Base_Buffer&& c) : m_ptr(NULL), m_size(0)
            {
                m_ptr = c.m_ptr;
                m_size = c.m_size;

                c.m_ptr = NULL;
                c.m_size = 0;
            }

            __host__ Base_Buffer& operator=(Base_Buffer&& c)
            {
                if(this != &c)
                {
                    _delete();

                    m_ptr = c.m_ptr;
                    m_size = c.m_size;
                    
                    c.m_ptr = NULL;
                    c.m_size = 0;
                }

                return *this;
            }

            typedef typename T* pointer;
            typedef typename const T& const_type_reference;
            typedef typename const pointer const_pointer;
            typedef typename T& type_reference;
            typedef typename Iterator<T, Base_Buffer<T, Content, Allocator>> iterator;
            typedef typename Iterator<const T, const Base_Buffer<T, Content, Allocator>> const_iterator;
            typedef typename size_t size_type;
            typedef typename const size_type& const_size_typ_reference;

            __host__ Base_Buffer(void)
            {
                m_size = 0;
                m_ptr = 0;
                m_pushBackIndex = 0;
            }

            __host__ Base_Buffer(size_type n)
            {
                m_size = 0;
                m_ptr = 0;
                m_pushBackIndex = 0;
                Resize(n);
            }

            __host__ Base_Buffer(size_type n, const_type_reference t)
            {
                m_size = 0;
                m_ptr = 0;
                m_pushBackIndex = 0;
                Resize(n);
                Fill(Begin(), End(), t);
            }

            __host__ iterator Begin(void)
            {
                //assert(m_size > 0);
                return iterator(m_ptr, this);
            }

            __host__ iterator End(void)
            {
                return iterator(m_ptr + m_size, this);
            }

            __host__ const_iterator Begin(void) const
            {
                return const_iterator(m_ptr, this);
            }

            __host__ const_iterator End(void) const
            {
                return const_iterator(m_ptr + m_size, this);
            }

            __host__ bool Empty(void)
            {
                return m_size == 0;
            }

            __host__ void Grow(size_type dn)
            {
                Resize(Size() + dn);
            }

            __host__ void Resize(size_type n)
            {
                if(n <= m_size)
                {
                    return;
                }

                T* old = m_ptr;
                size_type cpySize = n < m_size ? n : m_size; //(size_type)min(n, m_size);
                m_ptr = m_alloc.Allocate(n);
                if(old)
                {
                    const_iterator b(old, this);
                    const_iterator e(old + cpySize, this);
                    Copy(Begin(), b, e);
                    m_alloc.Deallocate(old);
                }
                m_size = n;
            }

            __host__ const_size_typ_reference Size(void) const
            {
                return m_size;
            }

            __host__ void Insert(iterator pos, const_type_reference v)
            {
                m_content.Insert(pos, v);
            }

            __host__ void Insert(iterator start, iterator end, const_type_reference v)
            {
                m_content.Insert(start, end, v);
            }

            __host__ void Insert(size_type pos, const_type_reference v)
            {
                Insert(Begin() + pos, v);
            }

//             void Insert(size_type pos, T v)
//             {
//                 m_content.Insert(Begin() + pos, v);
//             }

//             __host__ T operator[](const_iterator it) const
//             {
//                 return m_content[it];
//             }

            __host__ T operator[](size_type index) const
            {
#ifdef __DEBUG
                assert(index < m_size);
#endif
                //const_iterator it = Begin() + index;
                return m_content.Get(index, m_ptr);
            }

            __host__ const pointer* GetRawPointer(void) const
            {
                return &m_ptr;
            }

            __host__ void PushBack(const_type_reference v)
            {
                if(m_pushBackIndex >= m_size)
                {
                    Resize(Size() + 1);
                }
                Insert(m_pushBackIndex++, v);
            }

            template <
                class A,
                template <class A> class Ptr
            >
            __host__ void PushBack(const Ptr<A>& src, size_t d, cudaStream_t stream = 0)
            {
                if(GetPos() + d >= Size())
                {
                    Resize(Size() + d);
                }
                Copy(Begin() + GetPos(), src, d);
                //cudaMemcpyAsync(Begin()() + GetPos(), src(), d, cudaMemcpyHostToDevice, stream);
                m_pushBackIndex += d;
            }

            __host__ size_t GetPos(void)
            {
                return m_pushBackIndex;
            }

            __host__ void Reset(void)
            {
                m_pushBackIndex = 0;
            }

            __host__ size_t Bytes(void) const
            {
                return Size() * sizeof(T);
            }

            __host__ virtual ~Base_Buffer(void)
            {
                _delete();
            }

        protected:
            size_t m_size;
            size_t m_pushBackIndex;
            Allocator m_alloc;
            pointer m_ptr;
            Content m_content;
        };
    }
}