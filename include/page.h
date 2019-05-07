#ifndef PAGE_H
#define PAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

struct page {
    int p_num;
    int valid;
    unsigned long time_accessed;
};

struct node {
    int page_number;
    int replacement_chance;
    struct node* next;
};

struct pageTable {
    struct page** pages;
    struct node* head;
    int num_pages;
    int num_loaded;
};

long unsigned int page_swap_count;

struct pageTable* createPageTable(int, int);
struct page* createPage();
struct node* push(struct pageTable*, int);
int popFifo(struct pageTable*);
int popClock(struct pageTable*);
int getNextInvalid(struct pageTable*, int);
int nextValidPageLRU(struct pageTable*);

#endif
