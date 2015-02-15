// map_lib
// A simple associative-array library for C
//
// License: MIT / X11
// Copyright (c) 2009 by James K. Lawless
// jimbo@radiks.net http://www.radiks.net/~jimbo
// http://www.mailsend-online.com
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "map_lib.h"


map_t *map_create() {
    map_t *m;
    m = (map_t *)malloc(sizeof(map_t));
    if(!m)
        return NULL;
    m->name = NULL;
    m->value = NULL;
    m->next = NULL;
    return m;
}


void map_free(map_t *map) {
    if(!map)
        return;

    map_t *m,  *mp;

    m = map;
    while(m !=  NULL) {
        if(m->name !=  NULL) {
            free(m->name);
        }
        if(m->value !=  NULL) {
            free(m->value);
        }
        mp = m;
        m = m->next;
        free(mp);
    }
}


int map_set(map_t *m, char *name, void *value) {
    map_t *map;

    if(m->name == NULL) {
        m->name = strdup(name);
        if(!m->name)
            return -1;
        m->value = value;
        m->next = NULL;
        return 0;
    }

    for(map = m;; map = map->next) {
        if(strcmp(name, map->name) == 0) {
            if(map->value !=  NULL) {
                free(map->value);
                map->value = value;
                return 0;
            }
        }
        if(map->next == NULL) {
            map->next = (map_t *)malloc(sizeof(map_t));
            if(!map->next) {
                return -1;
            }
            map = map->next;
            map->name = strdup(name);
            if(!map->name) {
                return -1;
            }
            map->value = value;
            map->next = NULL;
            return 0;
        }
    }
    return -1;
}


void *map_get(map_t *m, char *name) {
    map_t *map;
    for(map = m; map !=  NULL; map = map->next) {
        if(map->name && strcmp(name, map->name) == 0) {
            return map->value;
        }
    }
    return NULL;
}


int map_len(map_t *m) {
    int counter;
    if(m == NULL) {
        return 0;
    }
    if(m->name == NULL) {
        return 0;
    }
    counter = 1;
    while(m->next != NULL) {
        m = m->next;
        counter++;
    }
    return counter;
}
