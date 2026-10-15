/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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
