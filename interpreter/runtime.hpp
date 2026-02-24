#ifndef RUNTIME_HPP
#define RUNTIME_HPP

extern "C"
{
#include "../runtime/gc.h"
#include "../runtime/runtime_common.h"

    extern size_t __gc_stack_top;
    extern size_t __gc_stack_bottom;

    aint Lread(void);
    aint Lwrite(aint n);

    aint LtagHash(char* s);
    char* de_hash(aint n);

    void* Bstring(char** p);
    void* Lstring(aint* args);
    aint Llength(void* p);

    void* Belem(void* p, aint i);
    void* Bsta(void* x, aint i, void* v);

    void* Barray(aint* args, aint bn);
    void* LmakeArray(aint length);

    void* Bsexp(aint* args, aint bn);
    aint Btag(void* d, aint t, aint n);

    void* Bclosure(aint* args, aint bn);

    aint Bclosure_tag_patt(void* x);
    aint Bstring_patt(void* x, void* y);
    aint Barray_patt(void* d, aint n);
    aint Bboxed_patt(void* x);
    aint Bunboxed_patt(void* x);
    aint Barray_tag_patt(void* x);
    aint Bstring_tag_patt(void* x);
    aint Bsexp_tag_patt(void* x);

    aint LkindOf(void* p);

    void failure(char const* s, ...);
}

#define INITIAL_STAGE_FAILURE(msg, ...) failure(msg, ##__VA_ARGS__)

#endif
