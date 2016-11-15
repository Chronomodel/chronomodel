#ifndef SINGLETON_H
#define SINGLETON_H

#include <QMutex>

template <class T>
class Singleton
{
public:
    static T* getInstance()
    {
        if(mInstance == NULL)
        {
            mMutex.lock();
            // we have to re-check if instance is null because it could have changed while the mutex was locked by other threads
            if(mInstance == NULL)
            {
                static bool alreadyInside = false;
                static bool createdOnceAlready = false;
                const bool problem = alreadyInside || (!mRecreateAfterDeletion && createdOnceAlready);
                Q_ASSERT(!problem);
                if(!problem)
                {
                    createdOnceAlready = true;
                    alreadyInside = true;
                    // use a stack variable to avoid setting the newObject value before the class has finished its constructor
                    T* newObject = new T();
                    alreadyInside = false;
                    mInstance = newObject;
                 }
            }
            mMutex.unlock();
        }
        return mInstance;
    }

    static inline T* getInstanceWithoutCreating()
    {
        return mInstance;
    }

    static void deleteInstance()
    {
        mMutex.lock();
        if(mInstance != NULL)
        {
            T* const old = mInstance;
            mInstance = NULL;
            delete old;
        }
        mMutex.unlock();
    }

    void clearSingletonInstance()
    {
        if(mInstance == this)
            mInstance = NULL;
    }

    bool canRecreateAfterDeletion() {return mRecreateAfterDeletion;}

public:
    static T* mInstance;
    static QMutex mMutex;
    static bool mRecreateAfterDeletion;

protected:
    Singleton(){} // hide constructor
    ~Singleton(){} // hide destructor
    Singleton(const Singleton &){} // hide copy constructor
    Singleton& operator=(const Singleton &){} // hide assign op
};

template <class T>
T* Singleton<T>::mInstance = NULL;
template <class T>
QMutex Singleton<T>::mMutex;
template <class T>
bool Singleton<T>::mRecreateAfterDeletion = false;

#endif
