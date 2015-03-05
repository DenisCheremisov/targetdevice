#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "maplib.h"


int if_theres_a_key(map_t *map, char *key) {

}

#define LENGTH 7


int main(int argc, char **argv) {
    map_t *map;
    map_iter_t *iter;
    map_item_t *item;
    char *res;
    int check, i;

    char *keys[LENGTH] = {
        "foo", "tom", "adam", "chip", "good", "poor", "chaos"
    };
    char *values[LENGTH] = {
        "bar", "jerry", "eva", "dale", "evil", "rich", "order"
    };
    int item_checked[LENGTH];

    map = map_create();
    for(i = 0; i < LENGTH; i++) {
        map_set(map, keys[i], values[i]);
        item_checked[i] = 0;
    }

    // Check iterator
    iter = map_iter(map);
    while(item = map_iter_next(iter), item != NULL) {
        for(i = 0; i < LENGTH; i++) {
            if(strcmp(item->key, keys[i]) == 0) {
                assert(strcmp(item->value, (char*)values[i]) == 0);
                item_checked[i] = 1;
            }
        }
        free(item);
    }
    free(iter);
    for(i = 0; i < LENGTH; i++) {
        assert(item_checked[i] == 1);
    }

    check =
        map_has(map, "foo") &&
        map_has(map, "tom") && map_has(map, "adam") &&
        map_has(map, "chip") && map_has(map, "good") &&
        map_has(map, "poor") && map_has(map, "chaos");
    assert(map_len(map) ==  7);

    res = map_pop(map, "foo");
    assert(strcmp(res, "bar") == 0);
    check = map_has(map, "tom") && map_has(map, "adam") &&
        map_has(map, "chip") && map_has(map, "good") &&
        map_has(map, "poor") && map_has(map, "chaos");
    assert(check);
    assert(!map_has(map, "foo"));
    assert(map_len(map) == 6);

    res = map_pop(map, "chaos");
    assert(strcmp(res, "order") == 0);
    check = map_has(map, "tom") && map_has(map, "adam") &&
        map_has(map, "chip") && map_has(map, "good") &&
        map_has(map, "poor");
    assert(check);
    assert(!map_has(map, "chaos"));
    assert(map_len(map) == 5);

    res = map_pop(map, "chip");
    assert(strcmp(res, "dale") == 0);
    check = map_has(map, "tom") && map_has(map, "adam") &&
        map_has(map, "good") && map_has(map, "poor");
    assert(check);
    assert(!map_has(map, "chip"));
    assert(map_len(map) == 4);

    puts("Test done\n");
    return 0;
}
