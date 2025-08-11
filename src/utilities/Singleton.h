/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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
