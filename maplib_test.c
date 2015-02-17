#include <stdio.h>
#include <assert.h>
#include "maplib.h"


int if_theres_a_key(map_t *map, char *key) {

}


int main(int argc, char **argv) {
    map_t *map;
    char *res;
    int check;

    map = map_create();
    map_set(map, "foo", "bar");
    map_set(map, "tom", "jerry");
    map_set(map, "adam", "eva");
    map_set(map, "chip", "dale");
    map_set(map, "good", "evil");
    map_set(map, "poor", "rich");
    map_set(map, "chaos", "order");

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
