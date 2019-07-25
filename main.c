#include <stdio.h>
#include "btree.h"
#include "string.h"

static void bt_traverse(Column *column){
    printf("%d ", column->id);
}

static void btree(void){
    BTree *btree = btree_init(3);
    for (int i = 0; i < 30; ++i) {
        Column c;
        c.id = i + 1;
        strcpy(c.title, "100 Years of Solitude");
        btree_add(btree, &c);
    }

    btree_traverse(btree, bt_traverse);
    printf("\n");

    for (int j = 30; j > 0; --j) {
        Column *c = btree_get_by_id(btree, j);
        printf("%d : %s\n", c->id, c->title);
    }

    for (int k = 30; k > 0; --k) {
        btree_delete_by_id(btree, k);
    }
    btree_clear(btree);
}

int main() {
    btree();
    return 0;
}