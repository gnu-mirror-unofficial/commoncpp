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

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/typeref.h>
#include <ucommon/string.h>
#include <ucommon/thread.h>
#include <ucommon/arrayref.h>
#include <cstdlib>

namespace ucommon {

ArrayRef::Array::Array(arraytype_t arraymode, void *addr, size_t used) :
Counted(addr, used), ConditionalAccess()
{
    size_t index = 0;
    Counted **list = get();

    head = 0;
    type = arraymode;
    if(type == ARRAY)
        tail = size;
    else
        tail = 0;

    if(!used)
        return;

    while(index < used) {
        list[index++] = NULL;
    }
}

void ArrayRef::Array::dealloc()
{
    size_t index = 0;
    Counted **list = get();

    if(!size)
        return;

    while(index < size) {
        Counted *object = list[index];
        if(object) {
            object->release();
            list[index] = NULL;
        }
        ++index;
    }
    size = 0;
    Counted::dealloc();
}

size_t ArrayRef::Array::count(void)
{
    if(head <= tail)
        return tail - head;

    return (tail + size) - head;
}

TypeRef::Counted *ArrayRef::Array::get(size_t index)
{
    if(index >= size)
        return NULL;

    return (get())[index];
}

TypeRef::Counted *ArrayRef::Array::remove(size_t index)
{
    if(index >= size)
        return NULL;

    Counted *result = get(index);

    (get())[index] = NULL;
    return result;
}

void ArrayRef::Array::assign(size_t index, Counted *object)
{
    if(index >= size)
        return;

    if(object)
        object->retain();

    Counted *replace = get(index);
    if(replace)
        replace->release();

    (get())[index] = object;
}

ArrayRef::ArrayRef() :
TypeRef()
{
}

ArrayRef::ArrayRef(arraytype_t type, size_t size) :
TypeRef(create(type, size))
{
}

ArrayRef::ArrayRef(arraytype_t type, size_t size, TypeRef& object) :
TypeRef(create(type, size))
{
    size_t index = 0;
    Array *array = polystatic_cast<Array *>(ref);

    if(!array || !array->size)
        return;

    while(index < array->size) {
        array->assign(index++, object.ref);
    }
}

ArrayRef::ArrayRef(const ArrayRef& copy) :
TypeRef(copy)
{
}

void ArrayRef::reset(Counted *object)
{
    size_t index = 0;
    size_t max;
    Array *array = polystatic_cast<Array *>(ref);

    if(!array || !array->size || !object)
        return;

    switch(array->type) {
    case ARRAY:
        max = array->size;
        break;
    case FALLBACK:
        max = 1;
        break;
    default:
        max = 0;
    }

    array->lock();
    array->head = 0;
    array->tail = max;
    while(index < max) {
        array->assign(index++, object);
    }
    array->signal();
    array->unlock();
}

void ArrayRef::pop(void)
{
    bool popped = false;
    Array *array = polystatic_cast<Array *>(ref);

    if(!array || !array->size)
        return;

    array->lock();
    switch(array->type) {
    case STACK:
        if(array->head != array->tail) {
            if(array->tail == 0)
                array->tail = array->size;
            --array->tail;
            array->assign(array->tail, NULL);
            popped = true;
        }
        break;
    case FALLBACK:
        if(array->count() == 1)
            break;
    case QUEUE:
        if(array->head != array->tail) {
            array->assign(array->head, NULL);
            if(++array->head >= array->size)
                array->head = 0;
            popped = true;
        }
        break;
    default:
        break;
    }
    if(popped)
        array->signal();
    array->unlock();
}

void ArrayRef::reset(TypeRef& var)
{
    reset(var.ref);
}

void ArrayRef::clear(void)
{
    reset(nullptr);
}

ArrayRef::Array *ArrayRef::create(arraytype_t mode, size_t size)
{
    if(!size)
        return NULL;

    size_t s = sizeof(Array) + (size * sizeof(Counted *));
    caddr_t p = auto_release.allocate(s);
    return new(mem(p)) Array(mode, p, size);
}

bool ArrayRef::push(const TypeRef& object, timeout_t timeout)
{
    Array *array = polystatic_cast<Array *>(ref);
    if(!array || array->type == ARRAY)
        return false;

    array->lock();
    while(array->count() >= (array->size - 1)) {
        if(!array->waitSignal(timeout)) {
            array->unlock();
            return false;
        }
    }
    array->assign(array->tail, object.ref);
    if(++array->tail >= array->size)
        array->tail = 0;
    array->broadcast();
    array->unlock();
    return true;
}

size_t ArrayRef::count()
{
    size_t result = 0;
    Array *array = polystatic_cast<Array *>(ref);
    if(array) {
        array->lock();
        result = array->count();
        array->unlock();
    }
    return result;
}

void ArrayRef::push(const TypeRef& object)
{
    Array *array = polystatic_cast<Array *>(ref);
    if(!array || array->type == ARRAY)
        return;

    array->lock();
    while(array->count() >= (array->size - 1)) {
        array->waitSignal();
    }
    array->assign(array->tail, object.ref);
    if(++array->tail >= array->size)
        array->tail = 0;
    array->broadcast();
    array->unlock();
}

void ArrayRef::pull(TypeRef& object, timeout_t timeout)
{
    object.clear();
    Array *array = polystatic_cast<Array *>(ref);
    if(!array || array->type == ARRAY) {
        return;
    }

    array->lock();
    for(;;) {
        if(array->head != array->tail) {
            Counted *value = NULL;
            switch(array->type) {
            case STACK:
                if(array->tail == 0)
                    array->tail = array->size;
                --array->tail;
                value = array->remove(array->tail);
                break;
            case FALLBACK:
                if(array->count() == 1) {
                    value = array->get(array->head);
                    break;
                }
            case QUEUE:
                value = array->remove(array->head);
                if(++array->head == array->size)
                    array->head = 0;
                break;
            default:
                break;
            }
            if(value) {
                array->signal();
                array->unlock();
                object.ref = value;
                return;
            }
        }
        if(!array->waitBroadcast(timeout)) {
            array->unlock();
            object.clear();
            return;
        }
    }
}

void ArrayRef::pull(TypeRef& object)
{
    object.clear();
    Array *array = polystatic_cast<Array *>(ref);
    if(!array || array->type == ARRAY) {
        return;
    }

    array->lock();
    for(;;) {
        if(array->head != array->tail) {
            Counted *value = NULL;
            switch(array->type) {
            case STACK:
                if(array->tail == 0)
                    array->tail = array->size;
                --array->tail;
                value = array->remove(array->tail);
                break;
            case FALLBACK:
                if(array->count() == 1) {
                    value = array->get(array->head);
                    break;
                }
            case QUEUE:
                value = array->remove(array->head);
                if(++array->head == array->size)
                    array->head = 0;
                break;
            default:
                break;
            }
            if(value) {
                array->signal();
                array->unlock();
                object.ref = value;
                return;
            }
        }
        array->waitBroadcast();
    }
}

void ArrayRef::assign(size_t index, TypeRef& t)
{
    Array *array = polystatic_cast<Array *>(ref);
    if(!array || index >= array->size)
        return;

    assert(array->type == ARRAY);

    Counted *obj = t.ref;
    array->lock();
    index += array->head;
    if(index >= array->size)
        index -= array->size;
    array->assign(index, obj);
    array->unlock();
}

void ArrayRef::resize(size_t size)
{
    Array *current = polystatic_cast<Array *>(ref);
    size_t index = 0;

    if(current) {
        Array *array = create(current->type, size);
        current->lock();
        switch(array->type) {
        case ARRAY:
            while(index < size && index < current->size) {
                array->assign(index, current->get(index));
                ++index;
            }
            array->tail = size;
            break;
        default:
            array->head = array->tail = 0;
            break;
        }
        current->unlock();
        TypeRef::set(array);
    }
}

void ArrayRef::realloc(size_t size)
{
    Array *current = polystatic_cast<Array *>(ref);
    if(current)
        TypeRef::set(create(current->type, size));
}

bool ArrayRef::is(size_t index)
{
    if(get(index))
        return true;

    return false;
}

TypeRef::Counted *ArrayRef::get(size_t index)
{
    Array *array = polystatic_cast<Array*>(ref);
    if(!array)
        return NULL;

    if(index >= array->size || array->head == array->tail) {
        return NULL;
	}

    array->lock();
    index += array->head;
    if(array->head <= array->tail && index >= array->tail) {
        array->unlock();
        return NULL;
    }
    if(index >= array->size)
        index -= array->size;

    if(index >= array->tail) {
        array->unlock();
        return NULL;
    }

    Counted *object = array->get(index);
    array->unlock();
    return object;
}

} // namespace
