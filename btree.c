//
// Created by Once on 2019/7/22.
//

#include "btree.h"
#include <stdlib.h>
#include <string.h>

// 顺序查找结点关键字，每个结点最多关键字为2t-1，时间复杂度为O(2t-1)，即O(t)
static int seq_search(const Column *array[], const int len, const int value){
    int i = 0;
    while(i <= len - 1 && value > array[i]->id)
        i++;
    return i;
}

// 二分法查找结点上相同的关键字、确定儿子访问位置，每个结点关键字数为2t-1，时间复杂度为O(log(2t-1))，即O(log t)
static int binary_search(Column *array[], const int len, const int value){
    int start = 0, end = len - 1, index = 0, center = (start + end) / 2;
    while(start <= end){
        if(value == array[center]->id){
            index = center;
            break;
        }
        else if(value > array[center]->id){
            index = center + 1;
            center += 1;
            start = center;
        }
        else{
            index = center;
            center -= 1;
            end = center;
        }
        center = (start + end) / 2;
    }
    return index;
}

BTree *btree_init(unsigned int degree){
    BTree *btree = (BTree*)malloc(sizeof(BTree));
    if(!btree){
        perror("init b tree error.");
        return NULL;
    }
    btree->root = NULL;
    btree->size = 0;
    btree->degree = degree;
    return btree;
}

int btree_is_full(BTree *btree){
    BNode *node = (BNode*)malloc(sizeof(BNode));
    if(!node)
        return 1;
    free(node);
    return 0;
}

int btree_is_empty(BTree *btree){
    if(btree == NULL)
        return 1;
    return btree->size == 0;
}

static BNode *new_node(BTree *btree){
    BNode *node = (BNode*)malloc(sizeof(BNode));
    if(!node)
        return NULL;
    node->columns = (Column**)malloc(sizeof(Column*) * (2 * btree->degree - 1));
    node->children = (BNode**)malloc(sizeof(BNode*) * (2 * btree->degree));
    if(!node->columns){
        free(node);
        return NULL;
    }
    if(!node->children){
        free(node);
        free(node->columns);
        return NULL;
    }
    return node;
}

/**
 * 1、创建新结点n
 * 2、转移关键字到n
 * 3、转移儿子到n
 * 4、n添加作为parent的儿子
 * 5、将中间关键字添加到parent
 * */
static int split_child(BTree *btree, BNode *parent, int index){
    BNode *left = parent->children[index];
    BNode *right = new_node(btree);
    if(!right)
        return 0;
    right->leaf = left->leaf;
    for (int i = btree->degree; i < left->size; ++i) {
        right->columns[i - btree->degree] = left->columns[i];
    }
    if(!left->leaf){
        for (int i = btree->degree; i < left->size + 1; ++i) {
            right->children[i - btree->degree] = left->children[i];
        }
    }
    right->size = btree->degree - 1;
    left->size = btree->degree - 1;
    for (int k = parent->size; k > index; --k) {
        parent->columns[k] = parent->columns[k - 1];
    }
    parent->columns[index] = left->columns[btree->degree - 1];
    for (int j = parent->size + 1; j > index + 1; --j) {
        parent->children[j] = parent->children[j - 1];
    }
    parent->children[index + 1] = right;
    parent->size++;
    btree->size++;
    return 1;
}

static int add_none_full(BTree *btree, BNode *root, Column *column){
    int index = binary_search(root->columns, root->size, column->id);
    if(root->leaf){
        if(index < root->size && root->columns[index]->id == column->id){
            strcpy(root->columns[index]->title, column->title);
            return 1;
        }
        Column *c = (Column*)malloc(sizeof(Column));
        if(!c)
            return 0;
        c->id = column->id;
        strcpy(c->title, column->title);
        for (int i = root->size; i > index; --i) {
            root->columns[i] = root->columns[i - 1];
        }
        root->columns[index] = c;
        root->size++;
        return 1;
    }
    else{
        if(root->children[index]->size == 2*btree->degree - 1){
            if(split_child(btree, root, index)){
                if(column->id > root->columns[index]->id)
                    index++;
            }
            else
                return 0;
        }
        return add_none_full(btree, root->children[index], column);
    }
}

static int add_node(BTree *btree, BNode *root, Column *column){
    if(!root){
        root = new_node(btree);
        if(!root)
            return 0;
        Column *c = (Column*)malloc(sizeof(Column));
        if(!c)
            return 0;
        c->id = column->id;
        strcpy(c->title, column->title);
        root->leaf = 1;
        root->columns[0] = c;
        root->size = 1;
        btree->root = root;
        btree->size++;
        return 1;
    }
    else{
        if(root->size == 2*btree->degree - 1){
            BNode *node = new_node(btree);
            if(!node)
                return 0;
            node->leaf = 0;
            node->size = 0;
            btree->root = node;
            node->children[0] = root;
            btree->size++;
            if(split_child(btree, node, 0)){
                int i = 0;
                if(node->columns[0]->id < column->id)
                    i++;
                return add_none_full(btree, node->children[i], column);
            }
            return 0;
        }
        else
            return add_none_full(btree, root, column);
    }
}

int btree_add(BTree *btree, Column *column){
    if(btree == NULL || column == NULL)
        return 0;
    return add_node(btree, btree->root, column);
}

static int delete_node(BTree *btree, BNode *root, int id);
// A、子树最大值
static Column *node_max(BNode *root);
// B、子树最小值
static Column *node_min(BNode *root);
// C、合并右儿子和k到左儿子中
static int node_merge(BTree *btree, BNode *parent, int index);

// 1、删除叶子结点中的相应关键字
static int delete_leaf_node(BNode *leaf, int index){
    for (int i = index + 1; i < leaf->size; ++i) {
        leaf->columns[i - 1] = leaf->columns[i];
    }
    leaf->size--;
    return 1;
}

// 2、删除内部结点中的相应关键字
static int delete_none_leaf_node(BTree *btree, BNode *parent, int index, int id){
    if(parent->children[index]->size >= btree->degree){
        Column *max = node_max(parent->children[index]);
        parent->columns[index] = max;
        return delete_node(btree, parent->children[index], max->id);
    }
    else if(parent->children[index + 1]->size >= btree->degree){
        Column *min = node_min(parent->children[index + 1]);
        parent->columns[index] = min;
        return delete_node(btree, parent->children[index + 1], min->id);
    }
    else{
        node_merge(btree, parent, index);
        return delete_node(btree, parent, id);
    }
}

// C、合并右儿子和k到左儿子中
/**
 * 1、转移k到child中
 * 2、从parent中删除k
 * 3、从parent中删除sibling
 * 4、将sibling的关键字转移到child
 * 5、将sibling的儿子转移到child（如果是非叶子）
 * 6、释放sibling及其相关内容
 * */
static int node_merge(BTree *btree, BNode *parent, int index){
    BNode *child = parent->children[index];
    BNode *sibling = parent->children[index + 1];
    child->columns[btree->degree - 1] = parent->columns[index];
    child->size++;
    for (int i = index + 1; i < parent->size; ++i) {
        parent->columns[i - 1] = parent->columns[i];
    }
    for (int j = index + 2; j < parent->size + 1; ++j) {
        parent->children[j - 1] = parent->children[j];
    }
    parent->size--;
    for (int k = 0; k < sibling->size; ++k) {
        child->columns[k + child->size] = sibling->columns[k];
    }
    if(!sibling->leaf){
        for (int i = 0; i < sibling->size + 1; ++i) {
            child->children[i + child->size + 1] = sibling->children[i];
        }
    }
    child->size += sibling->size;
    if(!sibling->leaf)
        free(sibling->children);
//    free(sibling->columns);
    free(sibling);
    btree->size--;
    return 1;
}

/**
 * 1、将parent中第index-1关键字下移到child中
 * 2、将sibling中的最大关键字上移到parent第index-1
 * 3、将sibling最后一个儿子移到child中第0个（非叶子）
 * */
static int node_borrow_left(BNode *parent, int index){
    BNode *child = parent->children[index];
    BNode *sibling = parent->children[index - 1];
    for (int i = child->size; i > 0; --i) {
        child->columns[i] = child->columns[i - 1];
    }
    child->columns[0] = parent->columns[index - 1];
    if(!child->leaf){
        for (int i = child->size + 1; i > 0; --i) {
            child->children[i] = child->children[i - 1];
        }
        child->children[0] = sibling->children[sibling->size];
    }
    child->size++;
    parent->columns[index - 1] = sibling->columns[sibling->size - 1];
    sibling->size--;
    return 1;
}

/**
 * 1、将parent中第index关键字下移到child中最大位置
 * 2、将sibling中的最小关键字上移到parent第index
 * 3、将sibling第一个儿子移到child中最后一个（非叶子）
 * */
static int node_borrow_right(BNode *parent, int index){
    BNode *child = parent->children[index];
    BNode *sibling = parent->children[index + 1];
    child->columns[child->size] = parent->columns[index];
    if(!child->leaf){
        child->children[child->size + 1] = sibling->children[0];
        for (int i = 1; i < sibling->size + 1; ++i) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }
    child->size++;
    parent->columns[index] = sibling->columns[0];
    for (int j = 1; j < sibling->size; ++j) {
        sibling->columns[j - 1] = sibling->columns[j];
    }
    sibling->size--;
    return 1;
}

// 3、填满儿子结点（途径的儿子）至少t个关键字：向左借、向右借、合并左或右
static int fill_child(BTree *btree, BNode *parent, int index){
    if(index != 0 && parent->children[index - 1]->size >= btree->degree){
        return node_borrow_left(parent, index);
    }
    else if(index != parent->size && parent->children[index + 1]->size >= btree->degree){
        return node_borrow_right(parent, index);
    }
    else{
        if(index != parent->size)
            return node_merge(btree, parent, index);
        else
            return node_merge(btree, parent, index - 1); // 最后一个儿子合并到倒数第二个
    }
}

static int delete_node(BTree *btree, BNode *root, int id){
    int index = binary_search(root->columns, root->size, id);
    if(index < root->size && root->columns[index]->id == id){
        if(root->leaf)
            return delete_leaf_node(root, index);
        else {
            return delete_none_leaf_node(btree, root, index, id);
        }
    }
    else{
        if(root->leaf)
            return 0;
        int end = (index == root->size) ? 1 : 0; // 最后一个儿子
        if(root->children[index]->size < btree->degree)
            fill_child(btree, root, index);
        if(end && index > root->size)
            return delete_node(btree, root->children[index - 1], id);
        else
            return delete_node(btree, root->children[index], id);
    }
}

int btree_delete_by_id(BTree *btree, int id){
    if(btree == NULL || btree->size == 0)
        return 0;
    BNode *root = btree->root;
    delete_node(btree, root, id);
    if(root->size == 0){
        if(!root->leaf)
            btree->root = root->children[0];
        else
            btree->root = NULL;
        free(root);
        btree->size--;
    }
    return 1;
}

static Column *node_max(BNode *root){
    while(!root->leaf)
        root = root->children[root->size];
    return root->columns[root->size - 1];
}

Column *btree_max(BTree *btree){
    if(btree == NULL || btree->size == 0)
        return NULL;
    return node_max(btree->root);
}

static Column *node_min(BNode *root){
    while(!root->leaf)
        root = root->children[0];
    return root->columns[0];
}

Column *btree_min(BTree *btree){
    if(btree == NULL || btree->size == 0)
        return NULL;
    return node_min(btree->root);
}

static Column *btree_get(BNode *root, int id){
    int index = binary_search(root->columns, root->size, id);
    if(index < root->size && root->columns[index]->id == id)
        return root->columns[index];
    else if(root->leaf)
        return NULL;
    else
        return btree_get(root->children[index], id);
}

Column *btree_get_by_id(BTree *btree, int id){
    if(btree == NULL || btree->size == 0)
        return NULL;
    return btree_get(btree->root, id);
}

static void traverse_tree(BNode *root, void(*traverse)(Column*)){
    int i;
    for (i = 0; i < root->size; ++i) {
        if(!root->leaf)
            traverse_tree(root->children[i], traverse);
        traverse(root->columns[i]);
    }
    if(!root->leaf)
        traverse_tree(root->children[i], traverse);
}

void btree_traverse(BTree *btree, void(*traverse)(Column*)){
    if(btree == NULL || btree->size == 0)
        return;
    traverse_tree(btree->root, traverse);
}

static int clear_node(BNode *root){
    if(!root)
        return 0;
    int i;
    for (i = 0; i < root->size; ++i) {
        if(!root->leaf)
            clear_node(root->children[i]);
        free(root->columns[i]);
    }
    if(!root->leaf){
        clear_node(root->children[i]);
        free(root->children);
    }
    free(root->columns);
    free(root);
    return 1;
}

int btree_clear(BTree *btree){
    if(btree == NULL)
        return 0;
    clear_node(btree->root);
    free(btree);
    return 1;
}
