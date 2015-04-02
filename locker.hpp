#ifndef _LOCKS_HPP_INCLUDED_
#define _LOCKS_HPP_INCLUDED_

#include <pthread.h>


template <class T>
class LockType {
public:
    static pthread_mutex_t mutex;
};
template <class T>
pthread_mutex_t LockType<T>::mutex = PTHREAD_MUTEX_INITIALIZER;


template <class T, class L>
class Locker {
private:
    LockType<L> locker;
    T *object;

public:
    Locker(T *obj) throw() {
        object = obj;
        pthread_mutex_lock(&(locker.mutex));
    }

    virtual ~Locker() throw() {
        pthread_mutex_unlock(&(locker.mutex));
    }

    T& operator*() const throw() {
        return *object;
    }

    T* operator->() const throw() {
        return object;
    }

};


template <class T>
class UnifiedLocker: public Locker<T, T> {
public:
    UnifiedLocker(T *obj): Locker<T, T>(obj) {};
};

#endif
