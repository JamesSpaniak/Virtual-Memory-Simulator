#include "../include/vm_sim.h"

/* Function declaration */
int checkPowerTwo(int);
int validateArguments(int, char**);
int getLineCount(FILE*);
int simulateVM(int, FILE*, FILE*);
int demandSwap(struct pageTable*, int, int, unsigned long);
int prePagingSwap(struct pageTable* tab, int, int, unsigned long);

/*
    Author(s): James Spaniak
    24 April 2019
*/

/* 
    Global program parameters 
    These variables corrspond to the 5 program arguments that the simulation takes in when started
*/
char* plist_path; /* path to plist file indicated by input */
char* ptrace_path; /* path to ptrace file indicated by input */
int page_size; /* page size for virtual memory */
char* page_replacement_algo; /* page replacement algorithm (LRU, FIFO, CLOCK) */
int pre_paging_demand; /* "+" -> pre-paging; "-" -> demand */

int main(int argc, char** argv) {
    FILE* plist_handle; /* file storage for plist_file */
    FILE* ptrace_handle; /* file storage for ptrace_file */
    int num_processes;

    if(validateArguments(argc, argv)!=0) {
        return 1;
    }

    plist_handle = fopen(plist_path, "r");
    if (plist_handle == NULL) {
        printf("Could not find plist file\n");
        return 1;
    }
    ptrace_handle = fopen(ptrace_path, "r");
    if (ptrace_handle == NULL) {
        printf("Could not find ptrace file\n");
        return 1;
    }
    num_processes = getLineCount(plist_handle);

    if(num_processes>0) {
        if(simulateVM(num_processes, plist_handle, ptrace_handle)!=0) {
            printf("Error, exiting simulation...\n");
            return 1;
        }
    }

    fclose(plist_handle);
    fclose(ptrace_handle);

    printf("Total page swaps: %lu\n", page_swap_count);
    return 0;

}
/*
    Function to validate inputs from main function and set global values
    argc: length of argument list input on command line
    argv: list of arguments passed via command line
    Return: 1 for invalid, 0 for valid
*/
int validateArguments(int argc, char** argv) {
    /* verifiy number of arguments that was input into program */
    if (argc != 6) {
        printf("Error: Ran as: %s <plist> <ptrace> <page_size> <page_replacement_algorithm> <pre-paging/demand-paging (+/-)>\n", argv[0]);
        return 1;
    }


    plist_path = argv[1];
    ptrace_path = argv[2];
    page_size = atoi(argv[3]);
    if(checkPowerTwo(page_size)==0 || page_size > MAX_PAGE_SIZE) {
        printf("Error: Page size must be from 2->32 and a power of 2.\n");
        return 1;
    }

    page_replacement_algo = argv[4];
    if(strcmp(page_replacement_algo, LRU) && strcmp(page_replacement_algo, FIFO) && strcmp(page_replacement_algo, CLOCK)) {
        printf("Error: Invalid algorithm, use one of the following: %s %s %s\n", LRU, FIFO, CLOCK);
        return 1;
    }

    if(strcmp(argv[5], "+") && strcmp(argv[5], "-")) {
        printf("Error: Use one of the following: + -> true; - -> false\n");
        return 1;
    } else if(strcmp(argv[5], "+")) {
        pre_paging_demand = 0;
    } else {
        pre_paging_demand = 1;
    }

    return 0;
}

/* 
    Function that takes in any variable and checks if it is a power of 2 or not
    value: int to be checked
    Return: 1 if power of 2; else 0
*/
int checkPowerTwo(int value) {
    if(value<1) {
        return 0;
    }
    if(value==1) {
        return 1;
    }
    /* Taken from: https://stackoverflow.com/questions/600293/how-to-check-if-a-number-is-a-power-of-2 */
    return (value & (value - 1)) == 0;
}

/*
    Function that takes in a FILE and returns a count of lines that are not blank
    Skips '\n' because we only want to count the proccess. Does NOT account for invalid format of individual lines.
    Returns non blank line count
*/
int getLineCount(FILE* in_file) {
    char curr = '\0';
    char prev;
    int lines = 0;
    int curr_char = 0;
    while(curr!=EOF) {
        prev = curr;
        curr = getc(in_file);
        curr_char++;
        if(curr=='\n') {
            //Want to exclude blank lines
            if(curr_char>0) {
                lines++;
                curr_char=0;
            }
        }
    }
    if(prev!='\n')
        lines++;
    return lines;
}

/*
    Virtual Memory simulation.
    Takes in number of processes, process list, and trace file
    Returns 0 on success, else 1
*/
int simulateVM(int num_processes, FILE* plist_handle, FILE* ptrace_handle) {
    struct pageTable** page_tables;
    struct node* curr_page;
    int i = 0, j = 0, cycle = 1, proc_num, memory_location, swap_check;
    char* line = NULL;
    char* temp;
    ssize_t ret;
    size_t len;
    int memory_per_process;

    if(fseek(plist_handle, 0L, SEEK_SET)!=0) { 
        printf("Error: Could not reposition pointer to beginning of file.\n");
        return 1;
    }

    page_tables = (struct pageTable**)malloc(sizeof(struct pageTable*)*num_processes);
    if(page_tables==NULL) {
        printf("Error: Could not allocate memory.\n");
        return 1;
    }
    //initialize page tables and all corresponding pages
    ret = getline(&line, &len, plist_handle);
    while(ret != -1) {
        if(strcmp(line, "\n")) {
            temp = strsep(&line, " ");
            if(line==NULL || strcmp(line, " ")==0) {
                printf("Error: Invalid Line format.\n");
                return 1;
            }
            page_tables[i] = createPageTable(page_size, atoi(strsep(&line, " ")));
            if(page_tables[i]==NULL) {
                printf("Error: Could not allocate page table memory.\n");
                return 1;
            }
        }
        ret = getline(&line, &len, plist_handle);
        i++;
    }
    //initialize initial memory
    memory_per_process = (int)floor(MEM_SIZE / (num_processes * page_size));
    for(i = 0; i < num_processes; i++) {
        for(j = 0; j < memory_per_process; j++) {
            page_tables[i]->pages[j]->valid=1;
            page_tables[i]->num_loaded++;

            if(strcmp(FIFO, page_replacement_algo)==0) {
                curr_page = push(page_tables[i], j);
                if(curr_page==NULL) {
                    printf("Error: Could not allocate page memory.");
                    return 1;
                }
            } else if(strcmp(CLOCK, page_replacement_algo)==0) {
                curr_page = push(page_tables[i], j);
                if(curr_page==NULL) {
                    printf("Error: Could not allocate page memory.");
                    return 1;
                }
            }
        }
    }

    //Begin core of simulation
    line = NULL;
    len = 0;
    ret = getline(&line, &len, ptrace_handle);
    while(ret != -1) {
        if(strcmp(line, "\n")) {
            proc_num = atoi(strsep(&line, " "));
            if(line==NULL || strcmp(line, " ")==0) {
                printf("Error: Invalid Line format.\n");
                return 1;
            }
            memory_location = atoi(strsep(&line, " "));
            memory_location = (int)ceil((double)memory_location/page_size);
            memory_location--;

            //Successful, the memory can be accessed
            if(page_tables[proc_num]->pages[memory_location]->valid) {
                if(strcmp(LRU, page_replacement_algo)==0) {
                    page_tables[proc_num]->pages[memory_location]->time_accessed=cycle;
                } else if(strcmp(CLOCK, page_replacement_algo)==0) {
                    if(page_tables[proc_num]->head!=NULL) {
                        curr_page=page_tables[proc_num]->head;
                        while(curr_page->page_number != memory_location && curr_page->next != NULL) {
                            curr_page=curr_page->next;
                        }
                        if(curr_page->page_number==memory_location) {
                            curr_page->replacement_chance=1;
                        }
                    }
                }
            //Missed, we need to swap
            } else {
                page_swap_count++;
                if(pre_paging_demand) {
                    //prepaging swap
                    swap_check = prePagingSwap(page_tables[proc_num], memory_location, memory_per_process, cycle);
                    if(swap_check==1) {
                        printf("Error: Could not swap pages.\n");
                        return 1;
                    }
                } else {
                    //demandpaging swap
                    swap_check = demandSwap(page_tables[proc_num], memory_location, memory_per_process, cycle);
                    if(swap_check==1) {
                        printf("Error: Could not swap pages.\n");
                        return 1;
                    }
                }
            }

            if(page_tables[proc_num]->num_loaded > memory_per_process) {
                printf("Error: Process %d exceeded memory available.\n", proc_num);
                return 1;
            }

            cycle++;

        }

        ret = getline(&line, &len, ptrace_handle);
    }

    return 0;
}

/*
    Demand Swap function, depending on page replacement algorithm function does different things
    Possible Algorithms: FIFO, LRU, CLOCK
    returns 1 on failure, else 0
*/
int demandSwap(struct pageTable* table, int mem_loc, int mem_limit, unsigned long cycle) {
    int page_num;
    struct node* check;
    if(strcmp(page_replacement_algo, FIFO)==0) {
        //pop first page
        page_num = popFifo(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }
        //load next page
        if(table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            check = push(table, mem_loc);
            if(check==NULL) {
                return 1;
            }
        }
    } else if(strcmp(page_replacement_algo, LRU)==0) {
        //first get the index of the page to be invalidated
        page_num = nextValidPageLRU(table);
        if(page_num > -1) {
            //invalidate said page from above
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }
        //load next page
        if(table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            table->pages[mem_loc]->time_accessed = cycle;
        }
    } else {
        //pop the next page to be invalidated
        page_num = popClock(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }
        //load page
        if(table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            check = push(table, mem_loc);
            if(check==NULL) {
                return -1;
            }
        }
    }
    return 0;
}

/*
    Prepaging Swap function, depending on page replacement algorithm function does different things
    Possible Algorithms: FIFO, LRU, CLOCK
    returns 1 on failure, else 0
*/
int prePagingSwap(struct pageTable* table, int mem_loc, int mem_limit, unsigned long cycle) {
    //all similar to demandSwap except you invalidate two pages and push back two on each algorithm
    int page_num;
    struct node* check;
    if(strcmp(page_replacement_algo, FIFO)==0) {
        page_num = popFifo(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }

        page_num = popFifo(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }
        
        if(table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            check = push(table, mem_loc);
            if(check==NULL) {
                return 1;
            }
        }

        mem_loc++;
        if(mem_loc==table->num_pages) {
            mem_loc=0;
        }
        page_num = getNextInvalid(table, mem_loc);
        if(page_num > -1 && table->num_loaded < mem_limit) {
            table->pages[page_num]->valid=1;
            table->num_loaded++;
            check = push(table, page_num);
            if(check==NULL) {
                return 1;
            }
        }

    } else if(strcmp(page_replacement_algo, LRU)==0) {
        page_num = nextValidPageLRU(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }
        
        page_num = nextValidPageLRU(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }

        if(table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            table->pages[mem_loc]->time_accessed = cycle;
        }

        mem_loc++;

        if(mem_loc==table->num_pages) {
            mem_loc=0;
        }

        mem_loc = getNextInvalid(table, mem_loc);
        if(mem_loc > -1 && table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            table->pages[mem_loc]->time_accessed=cycle;
        }
    } else {
        page_num = popClock(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }
        
        page_num = popClock(table);
        if(page_num > -1) {
            table->pages[page_num]->valid=0;
            table->num_loaded--;
        }

        if(table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            check = push(table, mem_loc);
            if(check==NULL) {
                return -1;
            }
        }

        mem_loc++;
        if(mem_loc==table->num_pages) {
            mem_loc=0;
        }

        mem_loc = getNextInvalid(table, mem_loc);
        if(mem_loc > -1 && table->num_loaded < mem_limit) {
            table->pages[mem_loc]->valid=1;
            table->num_loaded++;
            check = push(table, mem_loc);
            if(check==NULL) {
                return -1;
            }
        }
    }
    return 0;
}