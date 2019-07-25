//
// Created by Once on 2019/7/22.
//

#ifndef TREE_BTREE_H
#define TREE_BTREE_H

// 键值数组结构
typedef struct column{
    int id; // 关键字
    char title[128];
} Column;

// B树结点
typedef struct bnode{
    int size; // 当前关键字数目
    Column **columns; // 键值数组
    struct bnode **children; // 儿子指针数组
    unsigned int leaf; // 是否为叶子
} BNode;

// B树ADT对外接口
typedef struct btree{
    unsigned int degree; // 最小度数
    BNode *root; // 根结点
    unsigned int size; // B树结点大小
} BTree;

// B树算法操作声明
extern BTree *btree_init(unsigned int degree);
extern int btree_is_full(BTree *btree);
extern int btree_is_empty(BTree *btree);
extern int btree_add(BTree *btree, Column *column);
extern int btree_delete_by_id(BTree *btree, int id);
extern Column *btree_max(BTree *btree);
extern Column *btree_min(BTree *btree);
extern Column *btree_get_by_id(BTree *btree, int id);
extern void btree_traverse(BTree *btree, void(*traverse)(Column*));
extern int btree_clear(BTree *btree);

#endif //TREE_BTREE_H
