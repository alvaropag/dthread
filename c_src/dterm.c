
/******************************************************************************
 *
 *  Term (dterm_t)
 *
 *****************************************************************************/
#include <stddef.h>
#include <memory.h>
#include "../include/dterm.h"

static ErlDrvTermData am_true;
static ErlDrvTermData am_false;

void dterm_lib_init()
{
    am_true = driver_mk_atom("true");
    am_false = driver_mk_atom("false");
}


void dterm_init(dterm_t* p)
{
    p->dyn_alloc = 0;
    p->dyn_size  = DTERM_FIXED;
    p->base      = p->data;
    p->ptr       = p->data;
    p->ptr_end   = p->data + DTERM_FIXED;
    p->head      = 0;
    p->mark      = 0;
}

// dynamic allocation of dterm_t structure, the data part is
// can be less the DTERM_FIXED in this case
dterm_t* dterm_alloc(size_t size)
{
    size_t  sz = (sizeof(dterm_t) - DTERM_FIXED*sizeof(ErlDrvTermData)) +
	size*sizeof(ErlDrvTermData);
    dterm_t* p;

    if ((p = malloc(sz)) != NULL) {
	p->dyn_alloc  = 1;
	p->dyn_size   = size;
	p->base       = p->data;
	p->ptr        = p->data;
	p->ptr_end    = p->data + p->dyn_size;
	p->head       = 0;
	p->mark       = 0;
    }
    return p;
}

void dterm_reset_links(dterm_t* p)
{
    if (p->head) {
	dterm_link_t* lp = p->head;
	while(lp) {
	    dterm_link_t* nlp = lp->next;
	    driver_free(lp);
	    lp = nlp;
	}
	p->head = NULL;
    }
}

void dterm_finish(dterm_t* p)
{
    if (p->base != p->data)
	driver_free(p->base);
    dterm_reset_links(p);
}

void dterm_free(dterm_t* p)
{
    dterm_finish(p);
    if (p->dyn_alloc)
	driver_free(p);
}
    
// reset base pointer & clear link space
void dterm_reset(dterm_t* p)
{
    p->ptr = p->base;  // restart allocation
    dterm_reset_links(p);
}

int dterm_expand(dterm_t* p, size_t n)
{
    ErlDrvTermData* new_base;
    size_t old_size = dterm_allocated_size(p);
    size_t new_size = old_size + n;
    size_t old_sz   = old_size * sizeof(ErlDrvTermData);
    size_t new_sz   = new_size * sizeof(ErlDrvTermData);
    ptrdiff_t offset = p->ptr - p->base;  // offset of ptr

    if (p->base == p->data) {
	if ((new_base = driver_alloc(new_sz)) == NULL)
	    return 0;
	memcpy(new_base, p->base, old_sz);
    }
    else if ((new_base = driver_realloc(p->base, new_sz)) == NULL)
	return 0;
    p->base    = new_base;
    p->ptr     = p->base + offset;
    p->ptr_end = new_base + new_size;
    p->base    = new_base;
    return 1;
}

// auxillary space
void* dterm_link_alloc_data(dterm_t* p, size_t size)
{
    dterm_link_t* lp = driver_alloc(sizeof(dterm_link_t)+size);
    lp->next = p->head;
    p->head = lp;
    return (void*) &lp->data[0];
}

// auxillary space
void* dterm_link_copy_data(dterm_t* p, void* src, size_t size)
{
    void* dst = dterm_link_alloc_data(p, size);
    memcpy(dst, src, size);
    return dst;
}


void dterm_kv_int(dterm_t* t,ErlDrvTermData key, ErlDrvSInt value)
{
    dterm_mark_t m;
    dterm_tuple_begin(t, &m); {
	dterm_atom(t, key);
	dterm_int(t, value);
    }
    dterm_tuple_end(t, &m);
}

void dterm_kv_uint(dterm_t* t,ErlDrvTermData key, ErlDrvUInt value)
{
    dterm_mark_t m;
    dterm_tuple_begin(t, &m); {
	dterm_atom(t, key);
	dterm_uint(t, value);
    }
    dterm_tuple_end(t, &m);
}

void dterm_kv_atom(dterm_t* t,ErlDrvTermData key, ErlDrvTermData value)
{
    dterm_mark_t m;
    dterm_tuple_begin(t, &m); {
	dterm_atom(t, key);
	dterm_atom(t, value);
    }
    dterm_tuple_end(t, &m);
}

void dterm_kv_bool(dterm_t* t,ErlDrvTermData key, int value)
{
    dterm_mark_t m;
    dterm_tuple_begin(t, &m); {
	dterm_atom(t, key);
	dterm_atom(t, value ? am_true : am_false);
    }
    dterm_tuple_end(t, &m);
}

void dterm_kv_string(dterm_t* t,ErlDrvTermData key, char* value)
{
    dterm_mark_t m;
    size_t len = strlen(value);
    char* dst = (char*) dterm_link_copy_data(t, value, len);

    dterm_tuple_begin(t, &m); {
	dterm_atom(t, key);
	dterm_string(t, dst, len);
    }
    dterm_tuple_end(t, &m);
}
