// Copyright (C) 2006-2014 David Sugar, Tycho Softworks.
// Copyright (C) 2015-2020 Cherokees of Idaho.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

/**
 * Locking protocol classes for member function automatic operations.
 * This header covers ucommon access related classes.  These are used to
 * provide automatic management of locks and synchronization objects through
 * common virtual base classes which can be used with automatic objects.
 * These classes are related to "protocols" and are used in conjunction
 * with smart pointer/referencing classes.  The access interface supports
 * member functions to acquire a lock when entered and automatically
 * release the lock when the member function returns that are used in
 * conjunction with special referencing smart pointers.
 * @file ucommon/access.h
 */

// we do this twice because of some bizarre issue in just this file that
// otherwise breaks doxygen and lists all items outside the namespace...
#include <ucommon/platform.h>

#ifndef _UCOMMON_ACCESS_H_
#define _UCOMMON_ACCESS_H_

#ifndef _UCOMMON_CPR_H_
#include <ucommon/cpr.h>
#endif

#ifndef _UCOMMON_PROTOCOLS_H_
#include <ucommon/protocols.h>
#endif

namespace ucommon {

/**
 * An exclusive locking protocol interface base.
 * This is an abstract class to form objects that will operate under an
 * exclusive lock while being actively referenced by a smart pointer.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT ExclusiveProtocol
{
protected:
    virtual ~ExclusiveProtocol();

    virtual void _lock(void) = 0;

    virtual void _unlock(void) = 0;

public:
    /**
    * A kind of smart pointer object to support exclusive locking protocol.
    * This object initiates an exclusive lock for the object being referenced when
    * it is instantiated, and releases the exclusive lock when it is destroyed.
    * You would pass the pointer an object that has the Exclusive as a base class.
    * @author David Sugar <dyfet@gnutelephony.org>
    */
    class __EXPORT Locking
    {
    private:
        ExclusiveProtocol *lock;

        __DELETE_COPY(Locking);

    public:
        /**
        * Create an instance of an exclusive object reference.
        * @param object containing Exclusive base class protocol to lock.
        */
        Locking(ExclusiveProtocol *object);

        /**
        * Destroy reference to exclusively locked object, release lock.
        */
        ~Locking();

        /**
        * Test if the reference holds an active lock.
        * @return true if is not locking an object.
        */
        inline bool operator!() const {
            return lock == NULL;
        }

        /**
        * Test if the reference holds an active lock.
        * @return true if locking an object.
        */
        inline operator bool() const {
            return lock != NULL;
        }

        /**
        * Release a held lock programmatically.  This can be used to de-reference
        * the object being exclusively locked without having to wait for the
        * destructor to be called when the exclusive_lock falls out of scope.
        */
        void release(void);
    };
};

/**
 * An exclusive locking access interface base.
 * This is an abstract class to form objects that will operate under an
 * exclusive lock while being actively referenced by a smart pointer.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT SharedProtocol
{
protected:
    virtual ~SharedProtocol();

    /**
     * Access interface to share lock the object.
     */
    virtual void _share(void) = 0;

    virtual void _unshare(void) = 0;

public:
    /**
    * A kind of smart pointer object to support shared locking protocol.
    * This object initiates a shared lock for the object being referenced when
    * it is instantiated, and releases the shared lock when it is destroyed.
    * You would pass the pointer an object that has the Shared as a base class.
    * @author David Sugar <dyfet@gnutelephony.org>
    */
    class __EXPORT Locking
    {
    private:
        SharedProtocol *lock;
        int state;
        bool modify;

    public:
        /**
        * Create an instance of an exclusive object reference.
        * @param object containing Exclusive base class protocol to lock.
        */
        Locking(SharedProtocol *object);

        Locking(const Locking& copy);

        Locking& operator=(const Locking& copy);

        /**
        * Destroy reference to shared locked object, release lock.
        */
        ~Locking();

        /**
        * Test if the reference holds an active lock.
        * @return true if is not locking an object.
        */
        inline bool operator!() const {
            return lock == NULL;
        }

        /**
        * Test if the reference holds an active lock.
        * @return true if locking an object.
        */
        inline operator bool() const {
            return lock != NULL;
        }

        /**
        * Release a held lock programmatically.  This can be used to de-reference
        * the object being share locked without having to wait for the
        * destructor to be called when the shared_lock falls out of scope.
        */
        void release(void);

        /**
        * Call exclusive access on referenced objects protocol.
        */
        void exclusive(void);

        /**
        * Restore shared access on referenced objects protocol.
        */
        void share(void);
    };

    /**
     * Share the lock with other referencers.  Many of our shared locking
     * objects support the ability to switch between shared and exclusive
     * mode.  This derived protocol member allows one to restore the lock
     * to shared mode after it has been made exclusive.
     */
    virtual void share(void);

    /**
     * Convert object to an exclusive lock.  Many of our shared locking
     * objects such as the "conditional lock" support the ability to switch
     * between shared and exclusive locking modes.  This derived protocol
     * member allows one to temporarily assert exclusive locking when tied
     * to such methods.
     */
    virtual void exclusive(void);
};

/**
 * A kind of smart pointer object to support shared locking protocol.
 * This object initiates a shared lock for the object being referenced when
 * it is instantiated, and releases the shared lock when it is destroyed.
 * You would pass the pointer an object that has the Shared as a base class.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT shared_access
{
private:
    SharedProtocol *lock;
    int state;
    bool modify;

public:
    /**
     * Create an instance of an exclusive object reference.
     * @param object containing Exclusive base class protocol to lock.
     */
    shared_access(SharedProtocol *object);

    shared_access(const shared_access& copy);

    shared_access& operator=(const shared_access& copy);

    /**
     * Destroy reference to shared locked object, release lock.
     */
    ~shared_access();

    /**
     * Test if the reference holds an active lock.
     * @return true if is not locking an object.
     */
    inline bool operator!() const {
        return lock == NULL;
    }

    /**
     * Test if the reference holds an active lock.
     * @return true if locking an object.
     */
    inline operator bool() const {
        return lock != NULL;
    }

    /**
     * Release a held lock programmatically.  This can be used to de-reference
     * the object being share locked without having to wait for the
     * destructor to be called when the shared_lock falls out of scope.
     */
    void release(void);

    /**
     * Call exclusive access on referenced objects protocol.
     */
    void exclusive(void);

    /**
     * Restore shared access on referenced objects protocol.
     */
    void share(void);
};

template<class T>
class autoexclusive : private ExclusiveProtocol::Locking
{
private:
    __DELETE_DEFAULTS(autoexclusive);

public:
    inline autoexclusive(T *lock) :
    Locking(polystatic_cast<ExclusiveProtocol *>(lock)) {};
};

template<class T>
class autoshared : private SharedProtocol::Locking
{
private:
    __DELETE_DEFAULTS(autoshared);

public:
    inline autoshared(T *lock) :
    Locking(polystatic_cast<SharedProtocol *>(lock)) {};
};

// Special macros to allow member functions of an object with a protocol
// to create self locking states while the member functions are called by
// placing an exclusive_lock or shared_lock smart object on their stack
// frame to reference their self.

#define __EXCLUSIVE(x) exclusive_access __autolock__ = x
#define __SHARE(x) shared_access __autolock__ = x

} // namespace ucommon

#endif
