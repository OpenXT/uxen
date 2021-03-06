/******************************************************************************
 * domain_page.h
 * 
 * Allow temporary mapping of domain page frames into Xen space.
 * 
 * Copyright (c) 2003-2006, Keir Fraser <keir@xensource.com>
 */

#ifndef __XEN_DOMAIN_PAGE_H__
#define __XEN_DOMAIN_PAGE_H__

#include <xen/config.h>
#include <xen/mm.h>
#include <xen/perfc.h>
#include <xen/sched.h>
#include <uxen/mapcache.h>

#ifdef CONFIG_DOMAIN_PAGE

/*
 * Map a given page frame, returning the mapped virtual address. The page is
 * then accessible within the current VCPU until a corresponding unmap call.
 */
void *map_domain_page(unsigned long mfn);

/*
 * Pass a VA within a page previously mapped in the context of the
 * currently-executing VCPU via a call to map_domain_page().
 */
void unmap_domain_page(const void *va);

/*
 * Similar to the above calls, except the mapping is accessible in all
 * address spaces (not just within the VCPU that created the mapping). Global
 * mappings can also be unmapped from any context.
 */
void *map_domain_page_global(unsigned long mfn);
void unmap_domain_page_global(const void *va);

#define __map_domain_page(pg)        map_domain_page(__page_to_mfn(pg))
#define __map_domain_page_global(pg) map_domain_page_global(__page_to_mfn(pg))

#define DMCACHE_ENTRY_VALID 1U
#define DMCACHE_ENTRY_HELD  2U

struct domain_mmap_cache {
    unsigned long mfn;
    void         *va;
    unsigned int  flags;
};

static inline void
domain_mmap_cache_init(struct domain_mmap_cache *cache)
{
    ASSERT(cache != NULL);
    cache->flags = 0;
    cache->mfn = 0;
    cache->va = NULL;
}

static inline void *
map_domain_page_with_cache(unsigned long mfn, struct domain_mmap_cache *cache)
{
    ASSERT(cache != NULL);
    BUG_ON(cache->flags & DMCACHE_ENTRY_HELD);

    if ( likely(cache->flags & DMCACHE_ENTRY_VALID) )
    {
        cache->flags |= DMCACHE_ENTRY_HELD;
        if ( likely(mfn == cache->mfn) )
            goto done;
        unmap_domain_page_global(cache->va);
    }

    cache->mfn   = mfn;
    cache->va    = map_domain_page_global(mfn);
    cache->flags = DMCACHE_ENTRY_HELD | DMCACHE_ENTRY_VALID;

 done:
    return cache->va;
}

static inline void
unmap_domain_page_with_cache(const void *va, struct domain_mmap_cache *cache)
{
    ASSERT(cache != NULL);
    cache->flags &= ~DMCACHE_ENTRY_HELD;
}

static inline void
domain_mmap_cache_destroy(struct domain_mmap_cache *cache)
{
    ASSERT(cache != NULL);
    BUG_ON(cache->flags & DMCACHE_ENTRY_HELD);

    if ( likely(cache->flags & DMCACHE_ENTRY_VALID) )
    {
        unmap_domain_page_global(cache->va);
        cache->flags = 0;
    }
}

#else /* !CONFIG_DOMAIN_PAGE */

#ifndef __UXEN__

#define map_domain_page(mfn)                mfn_to_virt(mfn)
#define __map_domain_page(pg)               page_to_virt(pg)
#define unmap_domain_page(va)               ((void)(va))

#define map_domain_page_global(mfn)         mfn_to_virt(mfn)
#define __map_domain_page_global(pg)        page_to_virt(pg)
#define unmap_domain_page_global(va)        ((void)(va))

struct domain_mmap_cache { 
};

#define domain_mmap_cache_init(c)           ((void)(c))
#define map_domain_page_with_cache(mfn,c)   (map_domain_page(mfn))
#define unmap_domain_page_with_cache(va,c)  ((void)(va))
#define domain_mmap_cache_destroy(c)        ((void)(c))

#else   /* __UXEN__ */

static inline void *
uxen_map_page(unsigned long mfn)
{

#ifdef UXEN_HOST_WINDOWS
    return mapcache_map_page(mfn);
#else  /* UXEN_HOST_WINDOWS */
    return UI_HOST_CALL(ui_map_page_global, mfn);
#endif  /* UXEN_HOST_WINDOWS */
}

static inline void
uxen_unmap_page(const void *va)
{

#ifdef UXEN_HOST_WINDOWS
    mapcache_unmap_page_va(va);
#endif  /* UXEN_HOST_WINDOWS */
}

static inline void *
uxen_map_page_global(unsigned long mfn)
{
    void *_v;

    _v = UI_HOST_CALL(ui_map_page_global, mfn);
#ifdef DEBUG_MAPCACHE
    if (_v) {
        struct page_info *_pg;
        _pg = __mfn_to_page(mfn);
        atomic_inc(&_pg->mapped);
        _pg->lastmap = __builtin_return_address(0);
        _pg->lastmap0 = __builtin_return_address(1);
    }
#endif  /* DEBUG_MAPCACHE */
    return _v;
}

static inline void
uxen_unmap_page_global(const void *va)
{
#ifdef DEBUG_MAPCACHE
    unsigned long _mfn;

    _mfn = UI_HOST_CALL(ui_unmap_page_global_va, va);
    ASSERT(_mfn != INVALID_MFN);
    atomic_dec(&mfn_to_page(_mfn)->mapped);
#else  /* DEBUG_MAPCACHE */
    UI_HOST_CALL(ui_unmap_page_global_va, va);
#endif  /* DEBUG_MAPCACHE */
}

static inline void *
uxen_map_domain_page(unsigned long mfn)
{

    perfc_incr(map_domain_page_count);
    return uxen_map_page(mfn);
}

static inline void
uxen_unmap_domain_page(const void *va)
{

    perfc_incr(unmap_domain_page_count);
    uxen_unmap_page(va);
}

static inline void *
uxen_map_domain_page_global(unsigned long mfn)
{

    perfc_incr(map_domain_page_global_count);
    return uxen_map_page_global(mfn);
}

static inline void
uxen_unmap_domain_page_global(const void *va)
{

    perfc_incr(unmap_domain_page_global_count);
    uxen_unmap_page_global(va);
}

static inline void *
uxen_map_domain_page_direct(unsigned long mfn)
{

    perfc_incr(map_domain_page_direct_count);
    return uxen_map_page(mfn);
}

static inline void
uxen_unmap_domain_page_direct(const void *va)
{

    perfc_incr(unmap_domain_page_direct_count);
    uxen_unmap_page(va);
}

#define map_domain_page(mfn)                uxen_map_domain_page(mfn)
#define __map_domain_page(pg)               uxen_map_domain_page(__page_to_mfn(pg))
#define unmap_domain_page(va)               uxen_unmap_domain_page(va)
#ifndef UXEN_HOST_WINDOWS
#define mapped_domain_page_va_pfn(va)       virt_to_mfn(va)
#else  /* UXEN_HOST_WINDOWS */
#define mapped_domain_page_va_pfn(va)       mapcache_mapped_va_mfn(va)
#endif  /* UXEN_HOST_WINDOWS */

#define map_domain_page_direct(mfn)         uxen_map_domain_page_direct(mfn)
#define unmap_domain_page_direct(va)        uxen_unmap_domain_page_direct(va)

#define map_domain_page_global(mfn)         uxen_map_domain_page_global(mfn)
#define __map_domain_page_global(pg)        uxen_map_domain_page_global(__page_to_mfn(pg))
#define unmap_domain_page_global(va)        uxen_unmap_domain_page_global(va)

#if 0
struct domain_mmap_cache { 
};

#define domain_mmap_cache_init(c)           ((void)(c))
#define map_domain_page_with_cache(mfn,c)   (map_domain_page(mfn))
#define unmap_domain_page_with_cache(va,c)  ((void)(va))
#define domain_mmap_cache_destroy(c)        ((void)(c))
#endif

#endif  /* __UXEN__ */

#endif /* !CONFIG_DOMAIN_PAGE */

#endif /* __XEN_DOMAIN_PAGE_H__ */
