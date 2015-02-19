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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "maplib.h"


void unhandled_error() {
    perror("");
    exit(EXIT_FAILURE);
}


map_t *map_create() {
    map_t *m;
    m = (map_t *)malloc(sizeof(map_t));
    if(!m)
        return NULL;
    m->key = NULL;
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
        if(m->key !=  NULL) {
            free(m->key);
        }
        if(m->value !=  NULL) {
            free(m->value);
        }
        mp = m;
        m = m->next;
        free(mp);
    }
}


void map_set(map_t *m, char *key, void *value) {
    map_t *map;

    if(m->key == NULL) {
        m->key = strdup(key);
        if(!m->key)
            unhandled_error();
        m->value = value;
        m->next = NULL;
        return;
    }

    for(map = m;; map = map->next) {
        if(strcmp(key, map->key) == 0) {
            if(map->value !=  NULL) {
                free(map->value);
                map->value = value;
                return;
            }
        }
        if(map->next == NULL) {
            map->next = (map_t *)malloc(sizeof(map_t));
            if(!map->next) {
                unhandled_error();
            }
            map = map->next;
            map->key = strdup(key);
            if(!map->key) {
                unhandled_error();
            }
            map->value = value;
            map->next = NULL;
            return;
        }
    }
    unhandled_error();
}


void *map_get(map_t *m, char *key) {
    map_t *map;
    for(map = m; map !=  NULL; map = map->next) {
        if(map->key && strcmp(key, map->key) == 0) {
            return map->value;
        }
    }
    return NULL;
}


void *map_pop(map_t *m, char *key) {
    map_t *map, *prev, *tmp;
    void *result;
    result = NULL;
    prev = NULL;
    for(map = m; map !=  NULL; map = map->next) {
        if(map->key && strcmp(key, map->key) == 0) {
            result = map->value;
            if(map == m) {
                if(map->next == NULL) {
                    free(map->key);
                    map->key = NULL;
                    map->value = NULL;
                } else {
                    free(map->key);
                    map->key = map->next->key;
                    map->value = map->next->value;
                    tmp = map->next;
                    map->next = map->next->next;
                    free(tmp);
                }
            } else if (map->next == NULL) {
                prev->next = NULL;
                free(map->key);
                free(map);
            } else {
                prev->next = map->next;
                free(map->key);
                free(map);
            }
            break;
        }
        prev = map;
    }
    return result;
}


int map_has(map_t *m, char *key) {
    map_t *map;
    for(map = m; map != NULL; map = map->next) {
        if(map->key && strcmp(key, map->key) == 0) {
            return 1;
        }
    }
    return 0;
}


int map_len(map_t *m) {
    int counter;
    if(m == NULL) {
        return 0;
    }
    if(m->key == NULL) {
        return 0;
    }
    counter = 1;
    while(m->next != NULL) {
        m = m->next;
        counter++;
    }
    return counter;
}
