// SmartPtr.h

// raoul reyes 1996

// implementation of a smart pointer, a pointer who knows how to
// delete itself


#ifndef __SMARTPTR_H__
#define __SMARTPTR_H__

template <class T>

class smart_ptr 
{
public:
    smart_ptr( T* p = NULL );
    smart_ptr( smart_ptr& pCopy );
    smart_ptr& operator = ( smart_ptr& pAssign );
    T* Get() const;
    void operator = ( T* p );
    T* operator->() const;
    BOOL operator == ( T* );
    BOOL operator != ( T* );
    T& operator* () const;
    ~smart_ptr();


private:
    T* ptr;
};

//////////////////////////////////////////////////////////////////////////////////

template <class T>
smart_ptr< T >::smart_ptr( T* p )
{
    ptr = p;
}

template <class T>
smart_ptr< T >::smart_ptr( smart_ptr& pCopy )
{
    ptr = pCopy.ptr;
    pCopy.ptr = NULL;
}

template <class T>
smart_ptr< T >& smart_ptr< T >::operator = ( smart_ptr< T >& pAssign )
{
    ptr = pAssign.ptr;
    pAssign.ptr = NULL;
    return *this;
}

template <class T>
void smart_ptr< T >::operator = ( T* p )
{
    ptr = p;
}

template <class T>
smart_ptr< T >::~smart_ptr()
{
    delete ptr;
}

template <class T>
T* smart_ptr< T >::operator->() const
{
    return ptr;
}

template <class T>
T& smart_ptr< T >::operator* () const
{
    return *ptr;
}

template <class T>
T* smart_ptr< T >::Get() const
{
    return ptr;
}

template <class T>
BOOL smart_ptr< T >::operator == ( T* p)
{
    return ptr == p;
}

template <class T>
BOOL smart_ptr< T >::operator != ( T* p)
{
    return ptr != p;
}

#endif
