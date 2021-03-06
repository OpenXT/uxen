/******************************************************************************
 * include/asm-x86/paging.h
 *
 * physical-to-machine mappings for automatically-translated domains.
 *
 * Copyright (c) 2011 GridCentric Inc. (Andres Lagar-Cavilla)
 * Copyright (c) 2007 Advanced Micro Devices (Wei Huang)
 * Parts of this code are Copyright (c) 2006-2007 by XenSource Inc.
 * Parts of this code are Copyright (c) 2006 by Michael A Fetterman
 * Parts based on earlier work by Michael A Fetterman, Ian Pratt et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _XEN_P2M_H
#define _XEN_P2M_H

#include <xen/config.h>
#include <xen/paging.h>
#ifndef __UXEN__
#include <asm/mem_sharing.h>
#endif  /* __UXEN__ */
#include <asm/page.h>    /* for pagetable_t */

/*
 * The phys_to_machine_mapping maps guest physical frame numbers 
 * to machine frame numbers.  It only exists for paging_mode_translate 
 * guests. It is organised in page-table format, which:
 *
 * (1) allows us to use it directly as the second pagetable in hardware-
 *     assisted paging and (hopefully) iommu support; and 
 * (2) lets us map it directly into the guest vcpus' virtual address space 
 *     as a linear pagetable, so we can read and write it easily.
 *
 * For (2) we steal the address space that would have normally been used
 * by the read-only MPT map in a non-translated guest.  (For 
 * paging_mode_external() guests this mapping is in the monitor table.)
 */
#define phys_to_machine_mapping ((l1_pgentry_t *)RO_MPT_VIRT_START)

/*
 * The upper levels of the p2m pagetable always contain full rights; all 
 * variation in the access control bits is made in the level-1 PTEs.
 * 
 * In addition to the phys-to-machine translation, each p2m PTE contains
 * *type* information about the gfn it translates, helping Xen to decide
 * on the correct course of action when handling a page-fault to that
 * guest frame.  We store the type in the "available" bits of the PTEs
 * in the table, which gives us 8 possible types on 32-bit systems.
 * Further expansions of the type system will only be supported on
 * 64-bit Xen.
 */

/*
 * AMD IOMMU: When we share p2m table with iommu, bit 52 -bit 58 in pte 
 * cannot be non-zero, otherwise, hardware generates io page faults when 
 * device access those pages. Therefore, p2m_ram_rw has to be defined as 0.
 */
typedef enum {
    p2m_ram_rw = 0,             /* Normal read/write guest RAM */
    p2m_invalid = 1,            /* Nothing mapped here */
    p2m_ram_logdirty = 2,       /* Temporarily read-only for log-dirty */
    p2m_ram_ro = 3,             /* Read-only; writes are silently dropped */
    p2m_mmio_dm = 4,            /* Reads and write go to the device model */
    p2m_mmio_direct = 5,        /* Read/write mapping of genuine MMIO area */
    p2m_populate_on_demand = 6, /* Place-holder for empty memory */
    p2m_ram_immutable = 7,      /* Immutable page - warn on write */

#ifndef __UXEN__
    /* Although these are defined in all builds, they can only
     * be used in 64-bit builds */
    p2m_grant_map_rw = 7,         /* Read/write grant mapping */
    p2m_grant_map_ro = 8,         /* Read-only grant mapping */
    p2m_ram_paging_out = 9,       /* Memory that is being paged out */
    p2m_ram_paged = 10,           /* Memory that has been paged out */
    p2m_ram_paging_in = 11,       /* Memory that is being paged in */
    p2m_ram_paging_in_start = 12, /* Memory that is being paged in */
    p2m_ram_shared = 13,          /* Shared or sharable memory */
    p2m_ram_broken = 14,          /* Broken page, access cause domain crash */
#endif  /* __UXEN__ */
} p2m_type_t;

/*
 * Additional access types, which are used to further restrict
 * the permissions given my the p2m_type_t memory type.  Violations
 * caused by p2m_access_t restrictions are sent to the mem_event
 * interface.
 *
 * The access permissions are soft state: when any ambigious change of page
 * type or use occurs, or when pages are flushed, swapped, or at any other
 * convenient type, the access permissions can get reset to the p2m_domain
 * default.
 */
typedef enum {
    p2m_access_n     = 0, /* No access permissions allowed */
    p2m_access_r     = 1,
    p2m_access_w     = 2, 
    p2m_access_rw    = 3,
    p2m_access_x     = 4, 
    p2m_access_rx    = 5,
    p2m_access_wx    = 6, 
    p2m_access_rwx   = 7,
    p2m_access_rx2rw = 8, /* Special: page goes from RX to RW on write */

    /* NOTE: Assumed to be only 4 bits right now */
} p2m_access_t;

typedef enum {
    p2m_query,              /* Do not populate a PoD entries      */
    p2m_alloc,              /* Automatically populate PoD entries */
    p2m_alloc_r,            /* Automatically populate PoD entries for read */
    p2m_unshare,            /* Break c-o-w sharing; implies alloc */
    p2m_guest,              /* Guest demand-fault; implies alloc  */
    p2m_guest_r,            /* Guest demand-fault read access; implies alloc */
    p2m_zeroshare,          /* re-share gpfn with zero page */
    p2m_zeropop,            /* populate with a zeroed page */
} p2m_query_t;

#define p2m_query_to_mask(_q) (1UL << (_q))

#define P2M_GUEST_QUERY_MASK (p2m_query_to_mask(p2m_guest) |    \
                              p2m_query_to_mask(p2m_guest_r))
#define is_p2m_guest_query(q) (p2m_query_to_mask(q) & P2M_GUEST_QUERY_MASK)

#define P2M_ZEROING_QUERY_MASK (p2m_query_to_mask(p2m_zeroshare) |      \
                                p2m_query_to_mask(p2m_zeropop))
#define is_p2m_zeroing_any(q) (p2m_query_to_mask(q) & P2M_ZEROING_QUERY_MASK)
#define is_p2m_zeroshare(q) ((q) == p2m_zeroshare)
#define is_p2m_zeropop(q) ((q) == p2m_zeropop)

/* We use bitmaps and maks to handle groups of types */
#define p2m_to_mask(_t) (1UL << (_t))

/* RAM types, which map to real machine frames */
#ifndef __UXEN__
#define P2M_RAM_TYPES (p2m_to_mask(p2m_ram_rw)                \
                       | p2m_to_mask(p2m_ram_logdirty)        \
                       | p2m_to_mask(p2m_ram_ro)              \
                       | p2m_to_mask(p2m_ram_paging_out)      \
                       | p2m_to_mask(p2m_ram_paged)           \
                       | p2m_to_mask(p2m_ram_paging_in_start) \
                       | p2m_to_mask(p2m_ram_paging_in)       \
                       | p2m_to_mask(p2m_ram_immutable)       \
                       | p2m_to_mask(p2m_ram_shared))

/* Grant mapping types, which map to a real machine frame in another
 * VM */
#define P2M_GRANT_TYPES (p2m_to_mask(p2m_grant_map_rw)  \
                         | p2m_to_mask(p2m_grant_map_ro) )

/* MMIO types, which don't have to map to anything in the frametable */
#define P2M_MMIO_TYPES (p2m_to_mask(p2m_mmio_dm)        \
                        | p2m_to_mask(p2m_mmio_direct))

/* Read-only types, which must have the _PAGE_RW bit clear in their PTEs */
#define P2M_RO_TYPES (p2m_to_mask(p2m_ram_logdirty)     \
                      | p2m_to_mask(p2m_ram_ro)         \
                      | p2m_to_mask(p2m_grant_map_ro)   \
                      | p2m_to_mask(p2m_ram_shared) )

#define P2M_MAGIC_TYPES (p2m_to_mask(p2m_populate_on_demand))

/* Pageable types */
#define P2M_PAGEABLE_TYPES (p2m_to_mask(p2m_ram_rw))

#define P2M_PAGING_TYPES (p2m_to_mask(p2m_ram_paging_out)        \
                          | p2m_to_mask(p2m_ram_paged)           \
                          | p2m_to_mask(p2m_ram_paging_in_start) \
                          | p2m_to_mask(p2m_ram_paging_in))

#define P2M_PAGED_TYPES (p2m_to_mask(p2m_ram_paged))

/* Shared types */
/* XXX: Sharable types could include p2m_ram_ro too, but we would need to
 * reinit the type correctly after fault */
#define P2M_SHARABLE_TYPES (p2m_to_mask(p2m_ram_rw))
#define P2M_SHARED_TYPES   (p2m_to_mask(p2m_ram_shared))

/* Broken type: the frame backing this pfn has failed in hardware
 * and must not be touched. */
#define P2M_BROKEN_TYPES (p2m_to_mask(p2m_ram_broken))
#else  /* __UXEN__ */
#define P2M_RAM_TYPES (p2m_to_mask(p2m_ram_rw)                \
                       | p2m_to_mask(p2m_ram_logdirty)        \
                       | p2m_to_mask(p2m_ram_ro)              \
                       | p2m_to_mask(p2m_ram_immutable))

/* MMIO types, which don't have to map to anything in the frametable */
#define P2M_MMIO_TYPES (p2m_to_mask(p2m_mmio_dm)        \
                        | p2m_to_mask(p2m_mmio_direct))

/* Read-only types, which must have the _PAGE_RW bit clear in their PTEs */
#define P2M_RO_TYPES (p2m_to_mask(p2m_ram_ro))
#endif  /* __UXEN__ */

#define P2M_POD_TYPES (p2m_to_mask(p2m_populate_on_demand))

/* Useful predicates */
#ifndef __UXEN__
#define p2m_is_ram(_t) (p2m_to_mask(_t) & P2M_RAM_TYPES)
#define p2m_is_mmio(_t) (p2m_to_mask(_t) & P2M_MMIO_TYPES)
#define p2m_is_readonly(_t) (p2m_to_mask(_t) & P2M_RO_TYPES)
#define p2m_is_magic(_t) (p2m_to_mask(_t) & P2M_MAGIC_TYPES)
#define p2m_is_grant(_t) (p2m_to_mask(_t) & P2M_GRANT_TYPES)
#define p2m_is_grant_ro(_t) (p2m_to_mask(_t) & p2m_to_mask(p2m_grant_map_ro))
/* Grant types are *not* considered valid, because they can be
   unmapped at any time and, unless you happen to be the shadow or p2m
   implementations, there's no way of synchronising against that. */
#define p2m_is_valid(_t) (p2m_to_mask(_t) & (P2M_RAM_TYPES | P2M_MMIO_TYPES))
#define p2m_has_emt(_t)  (p2m_to_mask(_t) & (P2M_RAM_TYPES | p2m_to_mask(p2m_mmio_direct)))
#define p2m_is_pageable(_t) (p2m_to_mask(_t) & P2M_PAGEABLE_TYPES)
#define p2m_is_paging(_t)   (p2m_to_mask(_t) & P2M_PAGING_TYPES)
#define p2m_is_paged(_t)    (p2m_to_mask(_t) & P2M_PAGED_TYPES)
#define p2m_is_sharable(_t) (p2m_to_mask(_t) & P2M_SHARABLE_TYPES)
#define p2m_is_shared(_t)   (p2m_to_mask(_t) & P2M_SHARED_TYPES)
#define p2m_is_broken(_t)   (p2m_to_mask(_t) & P2M_BROKEN_TYPES)
#define p2m_is_pod(_t) (p2m_to_mask(_t) & P2M_POD_TYPES)
#else  /* __UXEN__ */
#define p2m_is_ram(_t) (p2m_to_mask(_t) & P2M_RAM_TYPES)
#define p2m_is_ram_rw(_t) (p2m_to_mask(_t) & p2m_to_mask(p2m_ram_rw))
// #define p2m_is_mmio(_t) (p2m_to_mask(_t) & P2M_MMIO_TYPES)
#define p2m_is_readonly(_t) (p2m_to_mask(_t) & P2M_RO_TYPES)
// #define p2m_is_grant(_t) 0
// #define p2m_is_grant_ro(_t) 0
#define p2m_is_valid(_t) ((_t) != p2m_invalid)
#define p2m_has_emt(_t)  (p2m_to_mask(_t) & (P2M_RAM_TYPES | p2m_to_mask(p2m_mmio_direct)))
// #define p2m_is_pageable(_t) 0
// #define p2m_is_paging(_t)   0
// #define p2m_is_paged(_t)    0
// #define p2m_is_sharable(_t) 0
// #define p2m_is_shared(_t)   0
// #define p2m_is_broken(_t)   0
#define p2m_is_pod(_t) (p2m_to_mask(_t) & P2M_POD_TYPES)
#define p2m_is_mmio_dm(_t) (p2m_to_mask(_t) & p2m_to_mask(p2m_mmio_dm))
#define p2m_is_mmio_direct(_t) (p2m_to_mask(_t) & p2m_to_mask(p2m_mmio_direct))
#define p2m_is_logdirty(_t) (p2m_to_mask(_t) & p2m_to_mask(p2m_ram_logdirty))
#define p2m_is_immutable(_t) (p2m_to_mask(_t) & p2m_to_mask(p2m_ram_immutable))
#endif  /* __UXEN__ */

#define p2m_update_pod_counts(d, omfn, ot, nmfn, nt) do {       \
        if (p2m_is_pod((ot))) {                                 \
            if (__mfn_zero_page((omfn)))                        \
                atomic_dec(&(d)->zero_shared_pages);            \
            else if (__mfn_valid_page_or_vframe((omfn)))        \
                atomic_dec(&(d)->tmpl_shared_pages);            \
            else if (__mfn_retry((omfn)))                       \
                atomic_dec(&(d)->retry_pages);                  \
            atomic_dec(&(d)->pod_pages);                        \
        }                                                       \
        if (p2m_is_pod((nt))) {                                 \
            if (__mfn_zero_page((nmfn)))                        \
                atomic_inc(&(d)->zero_shared_pages);            \
            else if (__mfn_valid_page_or_vframe((nmfn)))        \
                atomic_inc(&(d)->tmpl_shared_pages);            \
            else if (__mfn_retry((nmfn)))                       \
                atomic_inc(&(d)->retry_pages);                  \
            atomic_inc(&(d)->pod_pages);                        \
        }                                                       \
    } while (0)

union p2m_l1_cache {
    struct {
        /* prefix/table for set_entry l1 p2m table page cache */
        uint64_t se_l1_prefix;
        mfn_t se_l1_mfn;
        /* prefix/table/lock for get_entry l1 p2m table page cache */
        uint64_t ge_l1_prefix[NR_GE_L1_CACHE];
        mfn_t ge_l1_mfn[NR_GE_L1_CACHE];
#define ge_l1_cache_hash(gfn, p2m)                                      \
        ((((gfn) >> PAGETABLE_ORDER) + (p2m)->p2m_l1_cache_id) &        \
         (NR_GE_L1_CACHE - 1))
    };
};

#define p2m_l1_prefix(gfn, p2m)                   \
    (((uint64_t)(p2m)->p2m_l1_cache_id << 48) |   \
     ((uint64_t)(_atomic_read(p2m_l1_cache_gen) & \
                 P2M_L1_CACHE_GEN_MASK) << 40) |  \
     ((gfn) & ~((1UL << PAGETABLE_ORDER) - 1)))
DECLARE_PER_CPU(union p2m_l1_cache, p2m_l1_cache);
extern atomic_t p2m_l1_cache_gen;
#define P2M_L1_CACHE_GEN_MASK 0xff

void p2m_l1_cache_flush_softirq(void);
void p2m_ge_l1_cache_invalidate(struct p2m_domain *p2m, unsigned long gfn,
                                unsigned int page_order);

/* Per-p2m-table state */
struct p2m_domain {
    /* Lock that protects updates to the p2m */
    mm_lock_t          lock;

    /* Lock that protects updates to the logdirty table */
    mm_lock_t          logdirty_lock;

    /* Shadow translated domain: p2m mapping */
    pagetable_t        phys_table;

    /* Same as domain_dirty_cpumask but limited to
     * this p2m and those physical cpus whose vcpu's are in
     * guestmode.
     */
    cpumask_var_t      dirty_cpumask;

    struct domain     *domain;   /* back pointer to domain */

#ifndef __UXEN__
    /* Nested p2ms only: nested-CR3 value that this p2m shadows. 
     * This can be cleared to CR3_EADDR under the per-p2m lock but
     * needs both the per-p2m lock and the per-domain nestedp2m lock
     * to set it to any other value. */
#define CR3_EADDR     (~0ULL)
    uint64_t           cr3;

    /* Nested p2ms: linked list of n2pms allocated to this domain. 
     * The host p2m hasolds the head of the list and the np2ms are 
     * threaded on in LRU order. */
    struct list_head np2m_list; 


    /* Host p2m: when this flag is set, don't flush all the nested-p2m 
     * tables on every host-p2m change.  The setter of this flag 
     * is responsible for performing the full flush before releasing the
     * host p2m's lock. */
    int                defer_nested_flush;
#endif  /* __UXEN__ */

    /* Pages used to construct the p2m */
    struct page_list_head pages;

    int                (*set_entry   )(struct p2m_domain *p2m,
                                       unsigned long gfn,
                                       mfn_t mfn, unsigned int page_order,
                                       p2m_type_t p2mt,
                                       p2m_access_t p2ma);
    mfn_t              (*get_entry   )(struct p2m_domain *p2m,
                                       unsigned long gfn,
                                       p2m_type_t *p2mt,
                                       p2m_access_t *p2ma,
                                       p2m_query_t q,
                                       unsigned int *page_order);
    mfn_t              (*get_l1_table)(struct p2m_domain *p2m,
                                       unsigned long gpfn,
                                       unsigned int *page_order);
    mfn_t              (*parse_entry)(void *table, unsigned long index,
                                      p2m_type_t *t, p2m_access_t* a);
    int                (*write_entry)(struct p2m_domain *p2m, void *table,
                                      unsigned long gfn, mfn_t mfn, int target,
                                      p2m_type_t p2mt, p2m_access_t p2ma,
                                      int *needs_sync);
    int                (*split_super_page_one)(struct p2m_domain *p2m,
                                               void *entry, unsigned long gpfn,
                                               int order);
    void               (*change_entry_type_global)(struct p2m_domain *p2m,
                                                   p2m_type_t ot,
                                                   p2m_type_t nt);
    
#ifndef __UXEN__
    void               (*write_p2m_entry)(struct p2m_domain *p2m,
                                          unsigned long gfn, l1_pgentry_t *p,
                                          l1_pgentry_t new,
                                          unsigned int level);
#endif  /* __UXEN__ */

    int                (*ro_update_l2_entry)(struct p2m_domain *p2m,
                                             unsigned long gfn, int read_only,
                                             int *need_sync);

    /* Default P2M access type for each page in the the domain: new pages,
     * swapped in pages, cleared pages, and pages that are ambiquously
     * retyped get this access type.  See definition of p2m_access_t. */
    p2m_access_t default_access;

    /* If true, and an access fault comes in and there is no mem_event listener, 
     * pause domain.  Otherwise, remove access restrictions. */
    bool_t       access_required;

    bool_t virgin;

    /* controls whether asynchronous operations on the p2m are allowed */
    bool_t is_alive;

    /* Highest guest frame that's ever been mapped in the p2m */
    unsigned long max_mapped_pfn;

    unsigned long clone_gpfn;
    s64 clone_time;

    uint16_t p2m_l1_cache_id;

    struct dspage_store *dsps;

    union {
        struct {
            uint32_t gc_decompressed_gpfn;
#define GC_SCRUB_DELAY 30
            uint32_t gc_scrub_gpfn[GC_SCRUB_DELAY];
            struct timer gc_timer;
            short gc_per_iter;
            short gc_scrub_index;
            bool_t gc_was_preempted;
        } template;
    };

#ifndef NDEBUG
    unsigned long compress_gpfn;
#endif  /* NDEBUG */
};

/* get host p2m table */
#define p2m_get_hostp2m(d)      ((d)->arch.p2m)

/* Get p2m table (re)usable for specified cr3.
 * Automatically destroys and re-initializes a p2m if none found.
 * If cr3 == 0 then v->arch.hvm_vcpu.guest_cr[3] is used.
 */
struct p2m_domain *p2m_get_nestedp2m(struct vcpu *v, uint64_t cr3);

/* If vcpu is in host mode then behaviour matches p2m_get_hostp2m().
 * If vcpu is in guest mode then behaviour matches p2m_get_nestedp2m().
 */
struct p2m_domain *p2m_get_p2m(struct vcpu *v);

#define p2m_is_nestedp2m(p2m)   ((p2m) != p2m_get_hostp2m((p2m->domain)))

#define p2m_get_pagetable(p2m)  ((p2m)->phys_table)

/**** p2m query accessors. After calling any of the variants below, you
 * need to call put_gfn(domain, gfn). If you don't, you'll lock the
 * hypervisor. ****/

/* Read a particular P2M table, mapping pages as we go.  Most callers
 * should _not_ call this directly; use the other get_gfn* functions
 * below unless you know you want to walk a p2m that isn't a domain's
 * main one.
 * If the lookup succeeds, the return value is != INVALID_MFN and 
 * *page_order is filled in with the order of the superpage (if any) that
 * the entry was found in.  */
mfn_t get_gfn_type_access(struct p2m_domain *p2m, unsigned long gfn,
                    p2m_type_t *t, p2m_access_t *a, p2m_query_t q,
                    unsigned int *page_order);

/* General conversion function from gfn to mfn */
static inline mfn_t get_gfn_type(struct domain *d,
                                    unsigned long gfn, p2m_type_t *t,
                                    p2m_query_t q)
{
    p2m_access_t a;
    return get_gfn_type_access(p2m_get_hostp2m(d), gfn, t, &a, q, NULL);
}

/* Syntactic sugar: most callers will use one of these. 
 * N.B. get_gfn_query() is the _only_ one guaranteed not to take the
 * p2m lock; none of the others can be called with the p2m or paging
 * lock held. */
#define get_gfn(d, g, t)         get_gfn_type((d), (g), (t), p2m_alloc)
#define get_gfn_query(d, g, t)   get_gfn_type((d), (g), (t), p2m_query)
/* #define get_gfn_guest(d, g, t)   get_gfn_type((d), (g), (t), p2m_guest) */
#define get_gfn_unshare(d, g, t) get_gfn_type((d), (g), (t), p2m_unshare)

/* Compatibility function exporting the old untyped interface */
static inline unsigned long get_gfn_untyped(struct domain *d, unsigned long gpfn)
{
    mfn_t mfn;
    p2m_type_t t;
    mfn = get_gfn(d, gpfn, &t);
    if ( p2m_is_valid(t) )
        return mfn_x(mfn);
    return INVALID_MFN;
}

mfn_t
get_gfn_contents(struct domain *d, unsigned long gpfn, p2m_type_t *t,
                 uint8_t *buffer, uint32_t *size, int remove);

/* This is a noop for now. */
static inline void __put_gfn(struct p2m_domain *p2m, unsigned long gfn)
{
}

#define put_gfn(d, gfn) __put_gfn(p2m_get_hostp2m((d)), (gfn))

/* These are identical for now. The intent is to have the caller not worry 
 * about put_gfn. To only be used in printk's, crash situations, or to 
 * peek at a type. You're not holding the p2m entry exclsively after calling
 * this. */
#define get_gfn_unlocked(d, g, t)         get_gfn_type((d), (g), (t), p2m_alloc)
#define get_gfn_query_unlocked(d, g, t)   get_gfn_type((d), (g), (t), p2m_query)
#define get_gfn_guest_unlocked(d, g, t)   get_gfn_type((d), (g), (t),   \
                                                       p2m_guest_r)
#define get_gfn_unshare_unlocked(d, g, t) get_gfn_type((d), (g), (t), p2m_unshare)

#ifndef __UXEN__
/* General conversion function from mfn to gfn */
static inline unsigned long mfn_to_gfn(struct domain *d, mfn_t mfn)
{
    if ( paging_mode_translate(d) )
        return get_gpfn_from_mfn(mfn_x(mfn));
    else
        return mfn_x(mfn);
}
#endif  /* __UXEN__ */

/* Init the datastructures for later use by the p2m code */
int p2m_init(struct domain *d);

/* Mark p2m table alive -- this controls from when asynchronous
 * operations on the p2m are allowed */
int p2m_alive(struct domain *d);

/* Allocate a new p2m table for a domain. 
 *
 * Returns 0 for success or -errno. */
int p2m_alloc_table(struct p2m_domain *p2m);

/* Return all the p2m resources to Xen. */
void p2m_teardown(struct p2m_domain *p2m);
void p2m_final_teardown(struct domain *d);

/* Add a page to a domain's p2m table */
int guest_physmap_add_entry(struct domain *d, unsigned long gfn,
                            unsigned long mfn, p2m_type_t t);

/* Untyped version for RAM only, for compatibility */
static inline int guest_physmap_add_page(struct domain *d,
                                         unsigned long gfn,
                                         unsigned long mfn)
{
    return guest_physmap_add_entry(d, gfn, mfn, p2m_ram_rw);
}

/* Remove a page from a domain's p2m table */
void guest_physmap_remove_page(struct domain *d,
                               unsigned long gfn,
                               unsigned long mfn);

/* Set a p2m range as populate-on-demand */
int
guest_physmap_mark_pod_locked(struct domain *d, unsigned long gfn,
                              unsigned int order, mfn_t mfn);
int guest_physmap_mark_populate_on_demand(struct domain *d, unsigned long gfn,
                                          unsigned int order, mfn_t mfn);

/* Set a p2m range as populate-on-demand with contents */
int guest_physmap_mark_populate_on_demand_contents(
    struct domain *d, unsigned long gpfn, XEN_GUEST_HANDLE(uint8) buffer,
    unsigned int *pos);

/* Change types across all p2m entries in a domain */
void p2m_change_entry_type_global(struct domain *d, 
                                  p2m_type_t ot, p2m_type_t nt);

/* Change types across a range of p2m entries (start ... end-1) */
void p2m_change_type_range(struct domain *d, 
                           unsigned long start, unsigned long end,
                           p2m_type_t ot, p2m_type_t nt);
void p2m_change_type_range_l2(struct domain *d, 
                              unsigned long start, unsigned long end,
                              p2m_type_t ot, p2m_type_t nt);

/* Compare-exchange the type of a single p2m entry */
p2m_type_t p2m_change_type(struct domain *d, unsigned long gfn,
                           p2m_type_t ot, p2m_type_t nt);

/* Set mmio addresses in the p2m table (for pass-through) */
int set_mmio_p2m_entry(struct domain *d, unsigned long gfn, mfn_t mfn);
int clear_mmio_p2m_entry(struct domain *d, unsigned long gfn);


/* 
 * Populate-on-demand
 */

/* Dump PoD information about the domain */
void p2m_pod_dump_data(struct domain *d);

/* Move all pages from the populate-on-demand cache to the domain page_list
 * (usually in preparation for domain destruction) */
void p2m_pod_empty_cache(struct domain *d);

/* Set populate-on-demand cache size so that the total memory allocated to a
 * domain matches target */
int p2m_pod_set_mem_target(struct domain *d, unsigned long target);

void p2m_pod_final_free_pages(struct domain *d);

/* Call when decreasing memory reservation to handle PoD entries properly.
 * Will return '1' if all entries were handled and nothing more need be done.*/
int
p2m_pod_decrease_reservation(struct domain *d,
                             xen_pfn_t gpfn,
                             unsigned int order);

/* Scan pod cache when offline/broken page triggered */
int
p2m_pod_offline_or_broken_hit(struct page_info *p);

/* Replace pod cache when offline/broken page triggered */
void
p2m_pod_offline_or_broken_replace(struct page_info *p);

/* Clone one l1 table in p2m */
int
p2m_clone_l1(struct p2m_domain *op2m, struct p2m_domain *p2m,
             unsigned long gpfn, void *entry, int needs_sync);

/* Clone p2m */
int
p2m_clone(struct p2m_domain *p2m, struct domain *nd);

/* Remove page references on pages from other domains */
int
p2m_shared_teardown(struct p2m_domain *p2m);

void
p2m_teardown_compressed(struct p2m_domain *p2m);

void
p2m_pod_gc_template_pages_work(void *_d);


/*
 * Paging to disk and page-sharing
 */

#ifdef __x86_64__
/* Modify p2m table for shared gfn */
int set_shared_p2m_entry(struct domain *d, unsigned long gfn, mfn_t mfn);

/* Check if a nominated gfn is valid to be paged out */
int p2m_mem_paging_nominate(struct domain *d, unsigned long gfn);
/* Evict a frame */
int p2m_mem_paging_evict(struct domain *d, unsigned long gfn);
/* Tell xenpaging to drop a paged out frame */
void p2m_mem_paging_drop_page(struct domain *d, unsigned long gfn);
/* Start populating a paged out frame */
void p2m_mem_paging_populate(struct domain *d, unsigned long gfn);
/* Prepare the p2m for paging a frame in */
int p2m_mem_paging_prep(struct domain *d, unsigned long gfn);
/* Resume normal operation (in case a domain was paused) */
void p2m_mem_paging_resume(struct domain *d);
#else
static inline void p2m_mem_paging_drop_page(struct domain *d, unsigned long gfn)
{ }
static inline void p2m_mem_paging_populate(struct domain *d, unsigned long gfn)
{ }
#endif

#ifdef __x86_64__
/* Send mem event based on the access (gla is -1ull if not available).  Handles
 * the rw2rx conversion */
void p2m_mem_access_check(unsigned long gpa, bool_t gla_valid, unsigned long gla, 
                          bool_t access_r, bool_t access_w, bool_t access_x);
/* Resumes the running of the VCPU, restarting the last instruction */
void p2m_mem_access_resume(struct p2m_domain *p2m);

/* Set access type for a region of pfns.
 * If start_pfn == -1ul, sets the default access type */
int p2m_set_mem_access(struct domain *d, unsigned long start_pfn, 
                       uint32_t nr, hvmmem_access_t access);

/* Get access type for a pfn
 * If pfn == -1ul, gets the default access type */
int p2m_get_mem_access(struct domain *d, unsigned long pfn, 
                       hvmmem_access_t *access);

#else
static inline void p2m_mem_access_check(unsigned long gpa, bool_t gla_valid, 
                                        unsigned long gla, bool_t access_r, 
                                        bool_t access_w, bool_t access_x)
{ }
static inline int p2m_set_mem_access(struct domain *d, 
                                     unsigned long start_pfn, 
                                     uint32_t nr, hvmmem_access_t access)
{ return -EINVAL; }
static inline int p2m_get_mem_access(struct domain *d, unsigned long pfn, 
                                     hvmmem_access_t *access)
{ return -EINVAL; }
#endif

/* 
 * Internal functions, only called by other p2m code
 */

struct page_info *p2m_alloc_ptp(struct p2m_domain *p2m, unsigned long type);
void p2m_free_ptp(struct p2m_domain *p2m, struct page_info *pg);

#if CONFIG_PAGING_LEVELS == 3
static inline int p2m_gfn_check_limit(
    struct domain *d, unsigned long gfn, unsigned int order)
{
    /*
     * 32bit AMD nested paging does not support over 4GB guest due to 
     * hardware translation limit. This limitation is checked by comparing
     * gfn with 0xfffffUL.
     */
    if ( !hap_enabled(d) || ((gfn + (1ul << order)) <= 0x100000UL) ||
         (boot_cpu_data.x86_vendor != X86_VENDOR_AMD) )
        return (gfn + (1ul << order) - 1) < INVALID_GFN ? 0 : -EINVAL;

    if ( !test_and_set_bool(d->arch.hvm_domain.svm.npt_4gb_warning) )
        dprintk(XENLOG_WARNING, "vm%u failed to populate memory beyond"
                " 4GB: specify 'hap=0' domain config option.\n",
                d->domain_id);

    return -EINVAL;
}
#else
#define p2m_gfn_check_limit(d, gfn, order) \
    (((gfn) + (1ul << (order)) - 1) < INVALID_GFN ? 0 : -EINVAL)
#endif

/* Directly set a p2m entry: only for use by p2m code. Does not need
 * a call to put_gfn afterwards/ */
int set_p2m_entry(struct p2m_domain *p2m, unsigned long gfn, mfn_t mfn, 
                  unsigned int page_order, p2m_type_t p2mt, p2m_access_t p2ma);

/* Set up function pointers for PT implementation: only for use by p2m code */
extern void p2m_pt_init(struct p2m_domain *p2m);

/* Debugging and auditing of the P2M code? */
#define P2M_AUDIT     0
#define P2M_DEBUGGING 1

#if P2M_AUDIT
extern void audit_p2m(struct p2m_domain *p2m, int strict_m2p);
#else
# define audit_p2m(_p2m, _m2p) do { (void)(_p2m),(_m2p); } while (0)
#endif /* P2M_AUDIT */

/* Printouts */
#define P2M_PRINTK(_f, _a...)                                \
    debugtrace_printk("p2m: %s(): " _f, __func__, ##_a)
#define P2M_ERROR(_f, _a...)                                 \
    printk("pg error: %s(): " _f, __func__, ##_a)
#if P2M_DEBUGGING
#define P2M_DEBUG(_f, _a...)                                 \
    debugtrace_printk("p2mdebug: %s(): " _f, __func__, ##_a)
#else
#define P2M_DEBUG(_f, _a...) do { (void)(_f); } while(0)
#endif

int
p2m_get_compressed_page_data(struct domain *d, mfn_t mfn, uint8_t *data,
                             uint16_t offset, void *target, uint16_t *c_size);
int
_p2m_get_page_data(struct p2m_domain *p2m, mfn_t *mfn, uint8_t **data,
                   uint16_t *data_size, uint16_t *offset, int write_lock);
#define p2m_get_page_data(p2m, mfn, data, data_size, offset) \
    _p2m_get_page_data(p2m, mfn, data, data_size, offset, 0)
#define p2m_get_page_data_and_write_lock(p2m, mfn, data, data_size, offset) \
    _p2m_get_page_data(p2m, mfn, data, data_size, offset, 1)
void
_p2m_put_page_data(struct p2m_domain *p2m, uint8_t *data, uint16_t data_size,
                   int write_lock);
#define p2m_put_page_data(p2m, data, data_size) \
    _p2m_put_page_data(p2m, data, data_size, 0)
#define p2m_put_page_data_with_write_lock(p2m, data, data_size) \
    _p2m_put_page_data(p2m, data, data_size, 1)

/* Called by p2m code when demand-populating a PoD page */
mfn_t
p2m_pod_demand_populate(struct p2m_domain *p2m, unsigned long gfn,
                        unsigned int order, p2m_query_t q, void *entry);
/* Called by p2m code when re-sharing a zero page */
int
p2m_pod_zero_share(struct p2m_domain *p2m, unsigned long gfn,
                   p2m_query_t q, void *entry);

void
p2m_pod_free_page(struct page_info *page, va_list ap);

/*
 * Functions specific to the p2m-pt implementation
 */

/* Extract the type from the PTE flags that store it */
static inline p2m_type_t p2m_flags_to_type(unsigned long flags)
{
    /* For AMD IOMMUs we need to use type 0 for plain RAM, but we need
     * to make sure that an entirely empty PTE doesn't have RAM type */
    if ( flags == 0 ) 
        return p2m_invalid;
#ifdef __x86_64__
    /* AMD IOMMUs use bits 9-11 to encode next io page level and bits
     * 59-62 for iommu flags so we can't use them to store p2m type info. */
    return (flags >> 12) & 0x7f;
#else
    return (flags >> 9) & 0x7;
#endif
}

/*
 * Nested p2m: shadow p2m tables used for nested HVM virtualization 
 */

/* Flushes specified p2m table */
void p2m_flush(struct vcpu *v, struct p2m_domain *p2m);
/* Flushes all nested p2m tables */
void p2m_flush_nestedp2m(struct domain *d);

#ifndef __UXEN_NOT_YET__
void nestedp2m_write_p2m_entry(struct p2m_domain *p2m, unsigned long gfn,
    l1_pgentry_t *p, mfn_t table_mfn, l1_pgentry_t new, unsigned int level);
#endif  /* __UXEN_NOT_YET__ */

int
p2m_clear_gpfn_from_mapcache(struct p2m_domain *p2m, unsigned long gfn,
                             mfn_t mfn);

int
p2m_translate(struct domain *d, xen_pfn_t *arr, int nr, int write);

#define NPT_TABLE_ORDER 9
#define NPT_PAGETABLE_ENTRIES 512

#endif /* _XEN_P2M_H */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
