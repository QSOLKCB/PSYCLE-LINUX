/*
** PSYCLE-LINUX provenance-safe sort implementation.
**
** The r12005 path attributed its implementation to a published qsort
** example without carrying separate source terms. This replacement was
** independently written for PSYCLE-LINUX and uses heap sort while
** preserving the existing psy_qsort callback API.
**
** This source is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2, or (at your option)
** any later version.
*/

#include "qsort.h"

static void psy_sort_swap(void* container,
    psy_fp_set_index_double set,
    psy_fp_index_double get,
    intptr_t i, intptr_t j)
{
    void* tmp = get(container, (uintptr_t)i);
    set(container, (uintptr_t)i, get(container, (uintptr_t)j));
    set(container, (uintptr_t)j, tmp);
}

static void psy_sort_sift_down(void* container,
    psy_fp_set_index_double set,
    psy_fp_index_double get,
    intptr_t base, intptr_t count, intptr_t root,
    psy_fp_comp comp)
{
    for (;;) {
        intptr_t child = root * 2 + 1;
        intptr_t candidate = root;

        if (child >= count) {
            return;
        }
        if ((*comp)(get(container, (uintptr_t)(base + candidate)),
                get(container, (uintptr_t)(base + child))) < 0) {
            candidate = child;
        }
        if (child + 1 < count &&
            (*comp)(get(container, (uintptr_t)(base + candidate)),
                get(container, (uintptr_t)(base + child + 1))) < 0) {
            candidate = child + 1;
        }
        if (candidate == root) {
            return;
        }
        psy_sort_swap(container, set, get,
            base + root, base + candidate);
        root = candidate;
    }
}

void psy_qsort(void* container,
    psy_fp_set_index_double set,
    psy_fp_index_double get,
    intptr_t left, intptr_t right, psy_fp_comp comp)
{
    intptr_t count;
    intptr_t start;
    intptr_t end;

    if (left >= right) {
        return;
    }

    count = right - left + 1;
    start = (count - 2) / 2;
    for (;;) {
        psy_sort_sift_down(container, set, get,
            left, count, start, comp);
        if (start == 0) {
            break;
        }
        --start;
    }

    for (end = count - 1; end > 0; --end) {
        psy_sort_swap(container, set, get, left, left + end);
        psy_sort_sift_down(container, set, get,
            left, end, 0, comp);
    }
}
