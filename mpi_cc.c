/** Assignment 2: Basics of Parallel Programming with MPI
 * @author Samantha Preisig (ID: 1084328)
 * @date October 31, 2022
 * @file A2.c
 * @brief Implementation of a simple parallelized program that calculates and visualizes the character count for all letters in a text file
 * using MPI
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <limits.h>

#define MAX_BUFFER 1024
#define INIT_LINES 2 /*initial number of pointers/lines to allocate*/
#define ALPHABET 26

/** Counts the number of of times the letter occurs within the text
 * @param text 1D char array containing alpha text from the text file
 * @param start the starting index of the text covered by a given process
 * @param end the ending index of the text covered by a given process
 * @param letter the letter to be counted
 * @return on success: the number of times the letter occured within the text section (start-end)
**/
int count_letter(char *text, int start, int end, int letter){
    int i, counter = 0, ascii_lower_num = letter+65, ascii_higher_num = letter+97;
    for(i=start; i<end; i++){
        if(text[i] == ascii_lower_num || text[i] == ascii_higher_num){
            counter++;
        }
    }
    return counter;
}

/** Prints simple number counts to stdout or text file depending on s_flag
 * @param letter_counts 1D int array containing counts of occurence for each letter of the alphabet
 * @param s_flag command line flag indicating output to be saved to a text file
**/
void print_simple_count(int *letter_counts, int s_flag){
    FILE *fp = fopen("out.txt", "w");
    int i;
    char c;
    for(i=0; i<ALPHABET; i++){
        c = i+97;
        if(s_flag){
            fprintf(fp, "%d\n", letter_counts[i]);
        }else{
            printf("%c %d\n", c, letter_counts[i]);
        }
    }
    fclose(fp);
}

/** Prints letter histogram to stdout or text file depending on s_flag
 * Histogram visualization is scaled to 40
 * @param letter_counts 1D int array containing counts of occurence for each letter of the alphabet
 * @param max_count the highest letter count (used for scaling)
 * @param min_count the lowest letter count (used for scaling)
 * @param s_flag command line flag indicating output to be saved to a text file
**/
void print_letter_histogram(int *letter_counts, int max_count, int min_count, int s_flag){
    FILE *fp = fopen("out.txt", "w");
    int i, j;
    char c;
    for(i=0; i<ALPHABET; i++){
        c = i+97;

        if(letter_counts[i] <= 40 && letter_counts[i] != 0){
            for(j=0; j<letter_counts[i]; j++){
                if(s_flag){
                    fprintf(fp, "%c", c);
                }else{
                    printf("%c", c);
                }
            }
            if(s_flag){
                fprintf(fp, "\n");
            }else{
                printf("\n");
            }
        }
        if(letter_counts[i] == 0){
            if(s_flag){
                fprintf(fp, "\n");
            }else{
                printf("\n");
            }
        }
        if(letter_counts[i] > 40){
            /*Scale/normalize letter count*/
            /*normalize_n = [(n - min)/(max - min)]*40, where n is the count for the letter in question*/
            double norm_count = (letter_counts[i] - min_count);
            norm_count /= (max_count - min_count);
            norm_count *= 40;
            int norm_count_int = (int)norm_count; /*Scale down (truncate decimal)*/

            if(norm_count_int == 0){
                norm_count_int = 1; /*the letter still exists, so we print the letter*/
            }

            for(j=0; j<norm_count_int; j++){
                if(s_flag){
                    fprintf(fp, "%c", c);
                }else{
                    printf("%c", c);
                }
            }
            if(s_flag){
                fprintf(fp, "\n");
            }else{
                printf("\n");
            }
        }
    }
    fclose(fp);
}


/** Driver
 * Read command line arguments and store information
 * Initialize MPI processes and share data between processes using MPI_Send(), MPI_Recv(), and MPI_Reduce()
 * @param arg output arguments (saving to text file, simple count, histogram)
**/
int main(int argc, char **argv){

    /*MPI setup*/
    int comm_sz; /*number of processes*/
    int my_rank; /*my process rank*/
    /*Start up MPI*/
    MPI_Init(NULL, NULL);

    /*Get the number of processes*/
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    /*Get my rank among all the processes*/
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    MPI_Status status;

    /*Gathering information from command line*/
    char *filename = NULL;
    int l_flag=0, s_flag=0;
    
    if(argc == 4){
        l_flag=1; s_flag=1;
        filename = (char *)malloc(strlen(argv[3])*sizeof(char));
        strcpy(filename, argv[3]);
    }else if(argc == 3){
        if(strcmp(argv[1], "-l") == 0){
            l_flag=1;
        }else{
            s_flag=1;
        }
        filename = (char *)malloc(strlen(argv[2])*sizeof(char));
        strcpy(filename, argv[2]);
    }else{
        if(argc < 2){
            /*Shut down MPI and fprintf error*/
            MPI_Finalize();
            fprintf(stdout, "Usage: ./A2 [-l] [-s] filename\n");
            exit(0);
        }else{
            /*only filename is given*/
            filename = (char *)malloc(strlen(argv[1])*sizeof(char));
            strcpy(filename, argv[1]);
        }
    }

    FILE *fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    long size_of_file = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fclose(fp);

    char alpha_text[size_of_file];
    int index = 0; /*index count for alpha_text*/

    int *final_letter_counts = (int *)malloc(ALPHABET*sizeof(int));

    if(my_rank == 0){
        
        /*Character array loaded on the main process (my_rank == 0)*/
        char buffer[MAX_BUFFER], **raw_lines = NULL;
        size_t avail_pointers = INIT_LINES, used_ptrs = 0;
        fp = fopen(filename, "r");
        
        /*Error checking*/
        if(fp == NULL){
            perror("Failed to open file");
            return 1;
        }
        if((raw_lines = (char **)malloc(avail_pointers*sizeof(raw_lines))) == NULL){
            perror("malloc-lines");
            exit(EXIT_FAILURE);
        }
        
        /* Reading text file into 2D char array is heavily referenced from the following page:
         * https://stackoverflow.com/questions/66112939/how-to-read-a-text-file-and-store-in-an-array-in-c
        **/
        while(fgets(buffer, MAX_BUFFER, fp)){ /*read each line into buffer*/
            size_t len;
            buffer[(len = strcspn (buffer, "\n"))] = 0; /*triming newline and saving length*/
            
            if(used_ptrs == avail_pointers){ /*realloc raw_lines if needed*/
                void *tmp = realloc(raw_lines, (2*avail_pointers)*sizeof(raw_lines));
                if(tmp == NULL){
                    perror("realloc-lines");
                    break;
                }
                raw_lines = tmp;
                avail_pointers *= 2; /*updating number of raw_lines allocated*/
            }
            
            /*Error checking storage for line*/
            if(!(raw_lines[used_ptrs] = malloc(len+1))){
                perror("malloc-lines[used_ptrs]");
                break;
            }
            memcpy(raw_lines[used_ptrs], buffer, len+1); /*copy line from buffer to raw_lines[used_ptrs]*/
            used_ptrs += 1;
        }
        fclose(fp);
        free(filename);

        /*Building 1D char array with only alphabetical characters*/
        int row, col;
        index = 0;
        for(row = 0; row < used_ptrs; row++){
            for(col = 0; col < strlen(raw_lines[row]); col++){
                if((raw_lines[row][col] >= 65 && raw_lines[row][col] <= 90) || (raw_lines[row][col] >= 97 && raw_lines[row][col] <= 122)){
                    alpha_text[index] = raw_lines[row][col];
                    index++;
                }
            }
        }
        
        int q;
        for(q = 1; q < comm_sz; q++){            
            MPI_Send(alpha_text, index, MPI_CHAR, q, 1, MPI_COMM_WORLD); /*arbitrary tag = 1*/
        }
    }else{
        MPI_Recv(alpha_text, INT_MAX, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
    }

    /*Dividing the work amongst all processes*/
    int workload = 0;
    workload = strlen(alpha_text)/comm_sz;
    if(comm_sz*workload < strlen(alpha_text)){
        workload++;
    }
    int index_start = my_rank*workload, index_end = index_start+workload;
    
    /*Each process calculates letter counts for their section of the text*/
    int i, *letter_counts = (int *)malloc(ALPHABET*sizeof(int));
    for(i=0; i<ALPHABET; i++){
        letter_counts[i] = count_letter(alpha_text, index_start, index_end, i);
    }

    /*Sum all elements into final_letter_counts (from letter_counts each process has populated)*/
    MPI_Reduce(letter_counts, final_letter_counts, ALPHABET, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(my_rank == 0){
        /*Finding the highest and lowest letter counts for scaling calculations*/
        int max_count = final_letter_counts[0], min_count = final_letter_counts[0];
        for(i=0; i<ALPHABET; i++){
            if(final_letter_counts[i] > max_count){
                max_count = final_letter_counts[i];
            }
            if(final_letter_counts[i] < min_count){
                min_count = final_letter_counts[i];
            }
        }

        if(!l_flag){
            print_simple_count(final_letter_counts, s_flag);
        }else{
            print_letter_histogram(final_letter_counts, max_count, min_count, s_flag);
        }
    }
    free(letter_counts);
    free(final_letter_counts);
    
    /*Shut down MPI*/
    MPI_Finalize();
    return 0;
}