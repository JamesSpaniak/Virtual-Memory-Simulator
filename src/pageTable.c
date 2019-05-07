#include "../include/vm_sim.h"


/*
    Takes in page size and total memory and creates a pageTable including all pages within table setup
    using createPage function
    returns pageTable on success, else NULL
*/
struct pageTable* createPageTable(int page_size, int total_memory) {
    struct pageTable* page_table;
    struct page* curr_page;
    int i;
    int num_pages = (int)ceil((double)total_memory/page_size);

    page_table = (struct pageTable*)malloc(sizeof(struct pageTable));
    if(page_table==NULL) {
        return NULL;
    }
    page_table->pages = (struct page**)malloc(sizeof(struct page*)*num_pages);
    if(page_table->pages==NULL) {
        return NULL;
    }
    page_table->num_pages = num_pages;
    page_table->num_loaded = 0;
    for(i=0; i< num_pages; i++) {
        curr_page = createPage();
        if(curr_page==NULL) {
            return NULL;
        }
        page_table->pages[i] = curr_page;
    }
    page_table->head = NULL;

    return page_table;
}
/*
    Creates a page for use within pageTable
    return page on success, else NULL
*/
struct page* createPage() {
    struct page* page = (struct page*)malloc(sizeof(struct page));

    if(page==NULL) {
        return NULL;
    }

    if(!page_swap_count) {
        page_swap_count = 0;
    }

    page->p_num = ++page_swap_count;
    page->valid = 0;
    page->time_accessed = 0;

    return page;
}

/*
    Takes in relevant pageTable and page_number and pushes node object to end of queue
    Returns NULL on failure, node* on success.
*/
struct node* push(struct pageTable* table, int num) {
    struct node* curr;
    struct node* temp = (struct node*)malloc(sizeof(struct node));
    if(temp==NULL) {
        return NULL;
    }
    temp->page_number=num;
    temp->next = NULL;
    //For use with CLOCK, not relevant for other algorithmss
    temp->replacement_chance=1;
    if(table->head==NULL) {
        table->head=temp;
    } else {
        curr = table->head;
        while(curr->next!=NULL) {
            curr=curr->next;
        }
        curr->next = temp;
    }
    return temp;
}

/*
    For use with FIFO algorithm, gets top node from pageTable and returns page_num (-1 if queue is empty)
*/
int popFifo(struct pageTable* table) {
    int return_val = -1;
    struct node* curr;
    if(table->head==NULL) {
        return return_val;
    }

    curr=table->head->next;
    return_val=table->head->page_number;
    free(table->head);
    table->head = curr;

    return return_val;
}

/*
    For use with Clock Algorithm, pops first page that does not have a replacement_chance
    Clock is similar to FIFO except before being discarded each page gets a "second chance"
    return page number (-1 if list is empty)
*/

int popClock(struct pageTable* table) {
    int return_val = -1;
    struct node* curr, *prev;
    if(table->head==NULL) {
        return -1;
    }
    curr=table->head;
    
    while(1) {
        if(curr->replacement_chance) {
            curr->replacement_chance=0;

            if(table->head->next!=NULL) {
                table->head = curr->next;
            }

            prev = curr;
            while(prev->next != NULL) {
                prev = prev->next;
            }

            prev->next = curr;
            curr->next = NULL;

            curr = table->head;
        } else {
            return_val = curr->page_number;
            table->head = curr->next;
            free(curr);
            break;
        }
    }

    return return_val;
}

/*
    Loop over all pages starting at start index and return next page that is not valid
*/
int getNextInvalid(struct pageTable* table, int start) {
    int start_pos = start;

    while(table->pages[start]->valid) {
        start++;

        if(table->num_pages==start) {
            start=0;
        }
        if(start==start_pos) {
            return -1;
        }
    }
    
    return start;
}


/*
    For use with LRU algorithm;
    Loops through all pages and returns the next page to be swapped
    This corresponds to the least recently used page as indicated in time_accessed variable in the page struct
    returns index of page
*/
int nextValidPageLRU(struct pageTable* table) {
    int page, index = -1;
    unsigned long min_val = -1;

    for(page=0; page<table->num_pages; page++) {
        if(table->pages[page]->valid) {
            if(table->pages[page]->time_accessed < min_val) {
                min_val = table->pages[page]->time_accessed;
                index = page;
            }
        }
    }
    return index;
}