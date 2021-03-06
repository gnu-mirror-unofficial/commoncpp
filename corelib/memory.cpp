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

#include <ucommon-config.h>
#include <ucommon/export.h>
#include <ucommon/object.h>
#include <ucommon/memory.h>
#include <ucommon/thread.h>
#include <ucommon/string.h>
#include <ucommon/fsys.h>
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>
#include <string.h>
#include <stdio.h>

#if defined(_MSC_VER) && _MSC_VER >= 1800
#include <malloc.h>
#ifndef HAVE_ALIGNED_ALLOC
#define HAVE_ALIGNED_ALLOC 1
#endif
#define aligned_alloc(a, s) _aligned_malloc(s, a)
#endif

namespace ucommon {

extern "C" {

    static int ncompare(const void *o1, const void *o2)
    {
        assert(o1 != NULL);
        assert(o2 != NULL);
        const StringPager::member * const *n1 = static_cast<const StringPager::member * const*>(o1);
        const StringPager::member * const *n2 = static_cast<const StringPager::member * const*>(o2);
        return String::collate((*n1)->get(), (*n2)->get());
    }
}

void memalloc::assign(memalloc& source)
{
    memalloc::purge();
    pagesize = source.pagesize;
    align = source.align;
    count = source.count;
    page = source.page;
    limit = source.limit;
    source.count = 0;
    source.page = NULL;
}

memalloc::memalloc(size_t ps)
{
#ifdef  HAVE_SYSCONF
    size_t paging = sysconf(_SC_PAGESIZE);
#elif defined(PAGESIZE)
    size_t paging = PAGESIZE;
#elif defined(PAGE_SIZE)
    size_t paging = PAGE_SIZE;
#else
    size_t paging = 1024;
#endif
    if(!ps)
        ps = paging;
    else if(ps > paging)
        ps = (((ps + paging - 1) / paging)) * paging;

#if defined(HAVE_POSIX_MEMALIGN) || defined(HAVE_ALIGNED_ALLOC)
    if(ps >= paging)
        align = sizeof(void *);
    else
        align = 0;

    switch(align)
    {
    case 2:
    case 4:
    case 8:
    case 16:
        break;
    default:
        align = 0;
    }
#endif
    pagesize = ps;
    count = 0;
    limit = 0;
    page = NULL;
}

memalloc::memalloc(const memalloc& copy)
{
    count = 0;
    limit = 0;
    page = NULL;
    pagesize = copy.pagesize;
    align = copy.align;
}

memalloc::~memalloc()
{
    memalloc::purge();
}

unsigned memalloc::utilization(void) const
{
    unsigned long used = 0, alloc = 0;
    page_t *mp = page;

    while(mp) {
        alloc += (unsigned long)pagesize;
        used += mp->used;
        mp = mp->next;
    }

    if(!used)
        return 0;

    alloc /= 100;
    used /= alloc;
    return used;
}

void memalloc::purge(void)
{
    page_t *next;
    while(page) {
        next = page->next;
#if defined(HAVE_ALIGNED_ALLOC) && defined(_MSWINDOWS_)
        if (align)
            _aligned_free(page);
        else
            free(page);
#else
        free(page);
#endif
        page = next;
    }
    count = 0;
}

memalloc::page_t *memalloc::pager(void)
{
    page_t *npage = NULL;
#ifdef  HAVE_POSIX_MEMALIGN
    void *addr;
#endif

    if(limit && count >= limit) {
        __THROW_RUNTIME("pager exhausted");
        return NULL;
    }

#if defined(HAVE_POSIX_MEMALIGN)
    if(align && !posix_memalign(&addr, align, pagesize))
        npage = (page_t *)addr;
    else
        npage = (page_t *)malloc(pagesize);
#elif defined(HAVE_ALIGNED_ALLOC)
    if (align)
        npage = (page_t *)aligned_alloc(align, pagesize);
    else
        npage = (page_t *)malloc(pagesize);
#else
    npage = (page_t *)malloc(pagesize);
#endif

    if(!npage) {
    	__THROW_ALLOC();
        return NULL;
    }

    ++count;
    npage->used = sizeof(page_t);
    npage->next = page;
    page = npage;
    if((size_t)(npage) % sizeof(void *))
        npage->used += sizeof(void *) - ((size_t)(npage) % sizeof(void
*));
    return npage;
}

void *memalloc::_alloc(size_t size)
{
    assert(size > 0);

    caddr_t mem;
    page_t *p = page;

    if(size > (pagesize - sizeof(page_t))) {
        __THROW_SIZE("Larger than pagesize");
        return NULL;
    }

    while(size % sizeof(void *))
        ++size;

    while(p) {
        if(size <= pagesize - p->used)
            break;
        p = p->next;
    }
    if(!p)
        p = pager();

    mem = ((caddr_t)(p)) + p->used;
    p->used += (unsigned)size;
    return mem;
}

mempager::mempager(size_t ps) :
memalloc(ps)
{
    pthread_mutex_init(&mutex, NULL);
}

mempager::mempager(const mempager& copy) :
memalloc(copy)
{
    pthread_mutex_init(&mutex, NULL);
}

mempager::~mempager()
{
    memalloc::purge();
    pthread_mutex_destroy(&mutex);
}

void mempager::_lock(void)
{
    pthread_mutex_lock(&mutex);
}

void mempager::_unlock(void)
{
    pthread_mutex_unlock(&mutex);
}

unsigned mempager::utilization(void)
{
    unsigned long used;

    pthread_mutex_lock(&mutex);
    used = memalloc::utilization();
    pthread_mutex_unlock(&mutex);
    return used;
}

void mempager::purge(void)
{
    pthread_mutex_lock(&mutex);
    memalloc::purge();
    pthread_mutex_unlock(&mutex);
}

void mempager::dealloc(void *mem)
{
}

void *mempager::_alloc(size_t size)
{
    assert(size > 0);

    void *mem;
    pthread_mutex_lock(&mutex);
    mem = memalloc::_alloc(size);
    pthread_mutex_unlock(&mutex);
    return mem;
}

void mempager::assign(mempager& source)
{
    pthread_mutex_lock(&source.mutex);
    pthread_mutex_lock(&mutex);
    memalloc::assign(source);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_unlock(&source.mutex);
}


ObjectPager::member::member(LinkedObject **root) :
LinkedObject(root)
{
    mem = NULL;
}

ObjectPager::member::member() :
LinkedObject()
{
    mem = NULL;
}

ObjectPager::ObjectPager(size_t objsize, size_t size) :
memalloc(size)
{
    members = 0;
    root = NULL;
    last = NULL;
    index = NULL;
    typesize = objsize;
}

void ObjectPager::assign(ObjectPager& source)
{
    members = source.members;
    root = source.root;
    last = source.last;
    index = source.index;
    typesize = source.typesize;

    memalloc::assign(source);

    source.members = 0;
    source.root = NULL;
    source.last = NULL;
    source.index = NULL;
}

void *ObjectPager::get(unsigned ind) const
{
    linked_pointer<member> list = root;

    if(ind >= members)
        return invalid();

    while(ind--)
        list.next();

    return list->mem;
}

void ObjectPager::clear(void)
{
    memalloc::purge();
    members = 0;
    root = NULL;
    last = NULL;
    index = NULL;
}

void *ObjectPager::pull(void)
{
    if(!members)
        return invalid();

    member *mem = (member *)root;
    void *result = mem->mem;
    --members;
    if(!members) {
        root = NULL;
        last = NULL;
    }
    else
        root = mem->Next;
    index = NULL;
    return result;
}

void *ObjectPager::push(void)
{
    void *mem = memalloc::_alloc(sizeof(member));

    member *node;

    node = new(mem) member(&root);
	if (!node)
		return NULL;
    if(!last)
        last = node;
    ++members;
    node->mem = memalloc::_alloc(typesize);
    index = NULL;
    return node->mem;
}

void *ObjectPager::pop(void)
{
    void *out = NULL;

    if(!root)
        return invalid();

    index = NULL;

    if(root == last) {
        out = last->mem;
        root = last = NULL;
        members = 0;
        return out;
    }

    linked_pointer<member> np = root;
    while(is(np)) {
        if(np->Next == last) {
            out = last->mem;
            last = *np;
            np->Next = NULL;
            --members;
            break;
        }
        np.next();
    }
    return out;
}

void *ObjectPager::invalid(void) const
{
    return NULL;
}

void *ObjectPager::add(void)
{
    void *mem = memalloc::_alloc(sizeof(member));
    member *node;

    index = NULL;
    if(members++) {
        node = new(mem) member();
        last->set(node);
    }
    else
        node = new(mem) member(&root);
    last = node;
    node->mem = memalloc::_alloc(typesize);
    return node->mem;
}

void **ObjectPager::list(void)
{
    void **dp = index;
    if(dp)
        return dp;

    unsigned pos = 0;
    index = (void **)memalloc::_alloc(sizeof(void *) * (members + 1));
    linked_pointer<member> mp = root;
    while(is(mp)) {
        index[pos++] = mp->mem;
        mp.next();
    }
    index[pos] = NULL;
    return index;
}

StringPager::member::member(LinkedObject **root, const char *data) :
LinkedObject(root)
{
    text = data;
}

StringPager::member::member(const char *data) :
LinkedObject()
{
    text = data;
}

StringPager::StringPager(size_t size) :
memalloc(size)
{
    members = 0;
    root = NULL;
    last = NULL;
    index = NULL;
}

void StringPager::assign(StringPager& source)
{
    members = source.members;
    root = source.root;
    last = source.last;
    index = source.index;

    memalloc::assign(source);

    source.members = 0;
    source.root = NULL;
    source.last = NULL;
    source.index = NULL;
}

StringPager::StringPager(char **list, size_t size) :
memalloc(size)
{
    members = 0;
    root = NULL;
    last = NULL;
    add(list);
}

bool StringPager::filter(char *buffer, size_t size)
{
    add(buffer);
    return true;
}

unsigned StringPager::token(const char *text, const char *list, const char *quote, const char *end)
{
    unsigned count = 0;
    const char *result;
    char *lastp = NULL;

    if(!text || !*text)
        return 0;

    strdup_t tmp = strdup(text);
    while(NULL != (result = String::token(tmp, &lastp, list, quote, end))) {
        ++count;
        add(result);
    }
    return count;
}

String StringPager::join(const char *prefix, const char *middle, const char *suffix)
{
    string_t tmp;

    if(!members)
        return tmp;

    if(prefix && *prefix)
        tmp += prefix;

    linked_pointer<member> mp = root;
    while(is(mp)) {
        tmp += mp->text;
        if(mp->Next && middle && *middle)
            tmp += middle;
        else if(mp->Next == NULL && suffix && *suffix)
            tmp += suffix;
        mp.next();
    }

    return tmp;
}

unsigned StringPager::split(const char *text, const char *string, unsigned flags)
{
    strdup_t tmp = String::dup(string);
    char *match = tmp;
    char *prior = tmp;
    size_t tcl = strlen(text);
    unsigned count = 0;
    bool found = false;

    // until end of string or end of matches...
    while(prior && *prior && match) {
#if defined(_MSWINDOWS_)
        if((flags & 0x01) == String::INSENSITIVE)
            match = strstr(prior, text);
#elif  defined(HAVE_STRICMP)
        if((flags & 0x01) == String::INSENSITIVE)
            match = stristr(prior, text);
#else
        if((flags & 0x01) == String::INSENSITIVE)
            match = strcasestr(prior, text);
#endif
        else
            match = strstr(prior, text);

        if(match)
            found = true;

        // we must have at least one match to add trailing data...
        if(match == NULL && prior != NULL && found) {
            ++count;
            add(prior);
        }
        // if we have a current match see if anything to add in front of it
        else if(match) {
            *match = 0;
            if(match > prior) {
                ++count;
                add(prior);
            }

            prior = match + tcl;
        }
    }
    return count;
}

void StringPager::set(unsigned ind, const char *text)
{
    linked_pointer<member> list = root;

    if(ind >= members)

    while(ind--)
        list.next();

    size_t size = strlen(text) + 1;
    char *str = (char *)memalloc::_alloc(size);
#ifdef  HAVE_STRLCPY
    strlcpy(str, text, size);
#else
    strcpy(str, text);
#endif
    list->text = str;
}

const char *StringPager::get(unsigned ind) const
{
    linked_pointer<member> list = root;

    if(ind >= members) {
        __THROW_RANGE("stringpager outside range");
        return NULL;
    }

    while(ind--)
        list.next();

    return list->get();
}

void StringPager::clear(void)
{
    memalloc::purge();
    members = 0;
    root = NULL;
    last = NULL;
    index = NULL;
}

const char *StringPager::pull(void)
{
    if(!members) {
        __THROW_RUNTIME("no members");
        return NULL;
    }

    member *mem = (member *)root;
    const char *result = mem->text;
    --members;
    if(!members) {
        root = NULL;
        last = NULL;
    }
    else
        root = mem->Next;
    index = NULL;
    return result;
}

void StringPager::push(const char *text)
{
    if(!text)
        text = "";

    size_t size = strlen(text) + 1;
    void *mem = memalloc::_alloc(sizeof(member));
    char *str = (char *)memalloc::_alloc(size);

#ifdef  HAVE_STRLCPY
    strlcpy(str, text, size);
#else
    strcpy(str, text);
#endif
    member *node;

    node = new(mem) member(&root, str);
    if(!last)
        last = node;
    ++members;
    index = NULL;
}

const char *StringPager::pop(void)
{
    const char *out = NULL;

    if(root == nullptr) {
        __THROW_RUNTIME("no root");
        return NULL;
    }

    index = NULL;

    if(root == last) {
        out = last->text;
        root = last = NULL;
        members = 0;
        return out;
    }

    linked_pointer<member> np = root;
    while(is(np)) {
        if(np->Next == last) {
            out = last->text;
            last = *np;
            np->Next = NULL;
            --members;
            break;
        }
        np.next();
    }
    return out;
}

void StringPager::add(const char *text)
{
    if(!text)
        text = "";

    size_t size = strlen(text) + 1;
    void *mem = memalloc::_alloc(sizeof(member));
    char *str = (char *)memalloc::_alloc(size);

#ifdef  HAVE_STRLCPY
    strlcpy(str, text, size);
#else
    strcpy(str, text);
#endif
    member *node;

    index = NULL;
    if(members++) {
        node = new(mem) member(str);
        last->set(node);
    }
    else
        node = new(mem) member(&root, str);
    last = node;
}

void StringPager::set(char **list)
{
    clear();
    add(list);
}

void StringPager::push(char **list)
{
    const char *cp;
    unsigned ind = 0;

    if(!list)
        return;

    while(NULL != (cp = list[ind++]))
        push(cp);
}

void StringPager::add(char **list)
{
    const char *cp;
    unsigned ind = 0;

    if(!list)
        return;

    while(NULL != (cp = list[ind++]))
        add(cp);
}

void StringPager::sort(void)
{
    if(!members)
        return;

	unsigned count = members;
    member **list = new member*[members];
    unsigned pos = 0;
    linked_pointer<member> mp = root;

    while(is(mp) && count--) {
        list[pos++] = *mp;
        mp.next();
    }

    qsort(static_cast<void *>(list), members, sizeof(member *), &ncompare);
    root = NULL;
    while(pos)
        list[--pos]->enlist(&root);

    delete[] list;
    index = NULL;
}

char **StringPager::list(void)
{
    if(index)
        return index;

    unsigned pos = 0;
    index = (char **)memalloc::_alloc(sizeof(char *) * (members + 1));
    linked_pointer<member> mp = root;
    while(is(mp)) {
        index[pos++] = (char *)mp->text;
        mp.next();
    }
    index[pos] = NULL;
    return index;
}

DirPager::DirPager() :
StringPager()
{
    dir = NULL;
}

void DirPager::assign(DirPager& source)
{
    dir = source.dir;
    StringPager::assign(source);
    source.dir = NULL;
}

DirPager::DirPager(const char *path) :
StringPager()
{
    dir = NULL;
    load(path);
}

bool DirPager::filter(char *fname, size_t size)
{
    if(*fname != '.')
        add(fname);
    return true;
}

void DirPager::operator=(const char *path)
{
    dir = NULL;
    clear();
    load(path);
}

bool DirPager::load(const char *path)
{
    dir_t ds;
    char buffer[128];

    if(!fsys::is_dir(path))
        return false;

    dir = dup(path);
    ds.open(path);
    if(!ds)
        return false;

    while(ds.read(buffer, sizeof(buffer)) > 0) {
        if(!filter(buffer, sizeof(buffer)))
            break;
    }

    ds.close();
    sort();
    return true;
}

autorelease::autorelease()
{
    pool = NULL;
}

autorelease::~autorelease()
{
    release();
}

void autorelease::release()
{
    LinkedObject *obj;

    while(pool) {
        obj = pool;
        pool = obj->getNext();
        obj->release();
    }
}

void autorelease::operator+=(LinkedObject *obj)
{
    assert(obj != NULL);

    obj->enlist(&pool);
}

PagerObject::PagerObject() :
LinkedObject(nullptr), CountedObject()
{
}

void PagerObject::reset(void)
{
    CountedObject::reset();
    LinkedObject::Next = NULL;
}

void PagerObject::dealloc(void)
{
    pager->put(this);
}

void PagerObject::release(void)
{
    CountedObject::release();
}

void PagerObject::retain(void)
{
    CountedObject::retain();
}

PagerPool::PagerPool()
{
    freelist = NULL;
    pthread_mutex_init(&mutex, NULL);
}

PagerPool::~PagerPool()
{
    pthread_mutex_destroy(&mutex);
}

void PagerPool::put(PagerObject *ptr)
{
    assert(ptr != NULL);

    pthread_mutex_lock(&mutex);
    ptr->enlist(&freelist);
    pthread_mutex_unlock(&mutex);
}

PagerObject *PagerPool::get(size_t size)
{
    assert(size > 0);

    PagerObject *ptr;
    pthread_mutex_lock(&mutex);
    ptr = static_cast<PagerObject *>(freelist);
    if(ptr)
        freelist = ptr->Next;

    pthread_mutex_unlock(&mutex);

    if(!ptr)
        ptr = new((_alloc(size))) PagerObject;
    else
        ptr->reset();
	if (ptr)
		ptr->pager = this;
    return ptr;
}

} // namespace ucommon
