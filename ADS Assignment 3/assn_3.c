#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include<sys/time.h>

int inpsize;
int method;
int *inpbuf;
int *outbuf;

int create_sorted_runs(char *input_file_name);
int compare_func(const void *a, const void *b);
void merge_runs(int index, int run_count, FILE *outfile, char *run_file, int run_file_len);
void basic_merge(char *input_file_name, char *output_file_name);
void multistep_merge(char *input_file_name, char *output_file_name);
void replacement_selection_merge(char *input_file_name, char *output_file_name);
void sift(int *heap, int i, int n);
void heapify(int *heap, int num);

typedef struct
{
    FILE *files;
    int *ptr;
    int ind;
    int to_read;
} fptrs;

#define BUF_SIZE 1000
#define HSIZE 750
#define BSIZE 250

struct  timeval begin,end, elapsed;

void main(int argc, char *argv[])
{
    if(argc!=4)
    {
        printf("Incorrect Number of Arguments Provided");
        exit(1);
    }
    char * input_file_name = argv[2];
    char * output_file_name = argv[3];
    FILE *inp = fopen(input_file_name, "rb");
    if(inp == NULL)
    {
        printf("Input File does not exist");
        exit(1);
    }
    if(strcmp(argv[1], "--basic") == 0)
    {
        gettimeofday(&begin,NULL);
        basic_merge(input_file_name, output_file_name);
        gettimeofday(&end,NULL);
    }
    else if(strcmp(argv[1], "--multistep") == 0)
    {
        gettimeofday(&begin,NULL);
        multistep_merge(input_file_name, output_file_name);
        gettimeofday(&end,NULL);
    }
    else if(strcmp(argv[1], "--replacement") == 0)
    {
        gettimeofday(&begin,NULL);
        replacement_selection_merge(input_file_name, output_file_name);
        gettimeofday(&end,NULL);
    }
    else
    {
        printf("Invalid Argument Provided\n");
        exit(0);
    }

    elapsed.tv_sec=end.tv_sec-begin.tv_sec;
	elapsed.tv_usec=end.tv_usec-begin.tv_usec;
	if(elapsed.tv_usec< 0)
    {
		elapsed.tv_usec+=1000000;
		elapsed.tv_sec--;
    }
    printf( "Time: %ld.%06ld\n", elapsed.tv_sec, elapsed.tv_usec);
}


void basic_merge(char *input_file_name, char *output_file_name)
{
    int run_file_len, num_runs;
    char *run_file;
    FILE *outfile = fopen(output_file_name, "wb");
    run_file = (char *)malloc((strlen(input_file_name) + 4) * sizeof(char));
    run_file_len = strlen(input_file_name) + 4;
    strcpy(run_file, input_file_name);
    strcat(run_file, ".%03d");
    num_runs = create_sorted_runs(input_file_name);
    merge_runs(0, num_runs, outfile, run_file, run_file_len);
    free(run_file);
}

void multistep_merge(char *input_file_name, char *output_file_name)
{
    int i;
    int file_count = 15;
    int run_file_len = strlen(input_file_name) + 4;
    char *sfile = (char *)malloc((strlen(input_file_name) + 10) * sizeof(char));
    int num_runs = create_sorted_runs(input_file_name);
    int last_super_run = num_runs % 15;
    char *super_file = (char *)malloc((strlen(input_file_name) + 10) * sizeof(char));
    char *suffix_super = (char *)malloc(10 * sizeof(char));
    char *run_file = (char *)malloc((strlen(input_file_name) + 4) * sizeof(char));
    char *suffix = (char *)malloc(4 * sizeof(char));
    int super_run_num = num_runs / 15;
    int super_file_len = strlen(input_file_name) + 10;
    super_run_num += last_super_run == 0 ? 0 : 1;
    FILE *super_file_ptr, *out_file;
    strcpy(super_file, input_file_name);
    strcat(super_file, ".super.%03d");
    strcpy(run_file, input_file_name);
    strcat(run_file, ".%03d");
    for (i = 0; i < super_run_num; i++)
    {
        if (i == super_run_num - 1)
        {
            if (last_super_run != 0)
            {
                file_count = last_super_run;
            }
        }
        sprintf(sfile, super_file, i);
        super_file_ptr = fopen(sfile, "wb");
        merge_runs(0 + i * 15, file_count, super_file_ptr, run_file, run_file_len);
    }

    out_file = fopen(output_file_name, "wb");
    merge_runs(0, super_run_num, out_file, super_file, super_file_len);
    free(super_file);
    free(run_file);
    free(suffix_super);
    free(suffix);
    free(sfile);
}

int create_sorted_runs(char *input_file_name)
{
    FILE *run_ptr;
    int i, last_run;
    int buff_size;
    int num_runs;
    char *run_file = (char *)malloc((strlen(input_file_name) + 4) * sizeof(char));
    FILE *inpfile = fopen(input_file_name, "rb");
    fseek(inpfile, 0, SEEK_END);
    inpsize = ftell(inpfile) / sizeof(int);
    fseek(inpfile, 0, SEEK_SET);
    last_run = inpsize % BUF_SIZE;
    num_runs = inpsize / BUF_SIZE;
    num_runs += last_run == 0 ? 0 : 1;
    inpbuf = malloc(BUF_SIZE * sizeof(int));
    for (i = 0; i < num_runs; i++)
    {
        buff_size = BUF_SIZE;
        if (i == num_runs - 1)
        {
            if (last_run != 0)
            {
                buff_size = last_run;
            }
        }

        fread(inpbuf, sizeof(int), buff_size, inpfile);
        qsort(inpbuf, buff_size, sizeof(int), compare_func);
        sprintf(run_file, "%s.%03d", input_file_name, i);
        run_ptr = fopen(run_file, "wb+");
        fseek(run_ptr, 0, SEEK_SET);
        fwrite(inpbuf, sizeof(int), buff_size, run_ptr);
        fclose(run_ptr);
    }
    fclose(inpfile);
    free(inpbuf);
    return num_runs;
}

void merge_runs(int index, int run_count, FILE *outfile, char *run_file, int run_file_len)
{
    int i, finished = 0, optr = 0;
    int min_index = -1, min_val;
    char *file_name = (char *)malloc(run_file_len * sizeof(char));
    int buf_part = BUF_SIZE / run_count;
    fptrs file_ptrs[run_count];
    inpbuf = malloc(BUF_SIZE * sizeof(int));
    outbuf = malloc(BUF_SIZE * sizeof(int));

    for (i = 0; i < run_count; i++)
    {
        sprintf(file_name, run_file, i + index);
        file_ptrs[i].files = fopen(file_name, "rb");
        file_ptrs[i].ptr = inpbuf + i * buf_part;
        file_ptrs[i].to_read = fread(file_ptrs[i].ptr, sizeof(int), buf_part, file_ptrs[i].files);
        file_ptrs[i].ind = 0;
    }

    while (finished < run_count)
    {
        min_val = INT_MAX;
        min_index = -1;
        for (i = 0; i < run_count; i++)
        {
            if (file_ptrs[i].to_read != 0)
            {
                if (file_ptrs[i].ind != file_ptrs[i].to_read)
                {
                    if (*(file_ptrs[i].ptr + file_ptrs[i].ind) < min_val)
                    {
                        min_val = *(file_ptrs[i].ptr + file_ptrs[i].ind);
                        min_index = i;
                    }
                }
                else
                {
                    file_ptrs[i].to_read = fread(file_ptrs[i].ptr, sizeof(int), buf_part, file_ptrs[i].files);
                    file_ptrs[i].ind = 0;
                    if (file_ptrs[i].to_read == 0)
                    {
                        finished++;
                        fclose(file_ptrs[i].files);
                    }
                    else
                    {
                        if (*(file_ptrs[i].ptr + file_ptrs[i].ind) < min_val)
                        {
                            min_val = *(file_ptrs[i].ptr + file_ptrs[i].ind);
                            min_index = i;
                        }
                    }
                }
            }
        }
        if (min_index != -1)
        {
            outbuf[optr] = min_val;
            file_ptrs[min_index].ind++;
            optr++;
            if (optr >= BUF_SIZE)
            {
                optr = 0;
                fwrite(outbuf, sizeof(int), BUF_SIZE, outfile);
            }
        }
    }

    if (optr != 0)
    {
        fwrite(outbuf, sizeof(int), optr, outfile);
    }
    fclose(outfile);
    free(inpbuf);
    free(outbuf);
}

void replacement_selection_merge(char *input_file_name, char *output_file_name)
{
    FILE *curr;
    FILE *inpfile = fopen(input_file_name, "rb");
    fseek(inpfile, 0, SEEK_END);
    int input_size = (int)((size_t)ftell(inpfile) / sizeof(int));
    rewind(inpfile);
    if (input_size <= BUF_SIZE)
    {
        basic_merge(input_file_name, output_file_name);
        return;
    }
    FILE *outfile;
    char *run_file_pattern, *file_name, *index;
    int *heap = malloc(HSIZE * sizeof(int));
    int *buffer = malloc(BSIZE * sizeof(int));
    outbuf = malloc(BUF_SIZE * sizeof(int));
    int out_ind = 0, buf_ind = 0, heap_size = HSIZE, file_count = 0, run_len;
    int ending = 0, end_buff = 0, unread = input_size;
    file_name = malloc((strlen(input_file_name) + 4) * sizeof(char));
    index = malloc(4 * sizeof(char));
    sprintf(index, ".%03d", file_count);
    strcpy(file_name, input_file_name);
    strcat(file_name, index);
    curr = fopen(file_name, "wb");
    fread(heap, sizeof(int), HSIZE, inpfile);
    fread(buffer, sizeof(int), BSIZE, inpfile);
    unread = input_size - BUF_SIZE;
    heapify(heap, HSIZE);
    while (1)
    {
        if (ending ==0 || (ending!=0 && buf_ind != ending))
        {
            if (heap_size > 0)
            {
                outbuf[out_ind] = heap[0];
                out_ind = out_ind + 1;
                if (heap[0] > buffer[buf_ind])
                {
                    heap[0] = heap[heap_size - 1];
                    heap[heap_size - 1] = buffer[buf_ind];
                    buf_ind = buf_ind + 1;
                    heap_size = heap_size - 1;
                }
                else
                {
                    heap[0] = buffer[buf_ind];
                    buf_ind = buf_ind + 1;
                }
                if (buf_ind == BSIZE)
                {
                    int reads = (unread >= BSIZE ? BSIZE : unread);
                    unread = unread - reads;
                    if (unread == 0 && ending == 0)
                        ending = reads;
                    if (reads!=0)
                    {
                        fread(buffer, sizeof(int), reads, inpfile);
                        buf_ind = 0;
                    }
                }
                heapify(heap, heap_size);
                if (out_ind == BUF_SIZE)
                {
                    fwrite(outbuf, sizeof(int), BUF_SIZE, curr);
                    out_ind = 0;
                }
            }
            else
            {
                if (out_ind > 0)
                {
                    fwrite(outbuf, sizeof(int), out_ind, curr);
                    out_ind = 0;
                }
                fclose(curr);
                file_count = file_count + 1;
                sprintf(index, ".%03d", file_count);
                strcpy(file_name, input_file_name);
                strcat(file_name, index);
                curr = fopen(file_name, "wb");
                heap_size = HSIZE;
                heapify(heap, heap_size);
            }
        }
        else
        {
            if (heap_size > 0)
            {
                end_buff = end_buff + 1;
                outbuf[out_ind] = heap[0];
                out_ind = out_ind + 1;
                if (out_ind == BUF_SIZE)
                {
                    fwrite(outbuf, sizeof(int), BUF_SIZE, curr);
                    out_ind = 0;
                }
                heap[0] = heap[--heap_size];
                heapify(heap, heap_size);
            }
            else
            {
                if (out_ind > 0)
                {
                    fwrite(outbuf, sizeof(int), out_ind, curr);
                    out_ind = 0;
                }
                fclose(curr);
                file_count = file_count + 1;
                sprintf(index, ".%03d", file_count);
                strcpy(file_name, input_file_name);
                strcat(file_name, index);
                curr = fopen(file_name, "wb");
                qsort(&heap[end_buff], (size_t)(HSIZE - end_buff), sizeof(int), compare_func);
                fwrite(&heap[end_buff], sizeof(int), (HSIZE - end_buff), curr);
                fclose(curr);
                break;
            }
        }
    }
    if (out_ind > 0)
    {
        fwrite(outbuf, sizeof(int), out_ind, curr);
        fclose(curr);
    }
    outfile = fopen(output_file_name, "wb");
    run_len = strlen(input_file_name) + 4;
    run_file_pattern = (char *)malloc(run_len * sizeof(char));
    strcpy(run_file_pattern, input_file_name);
    strcat(run_file_pattern, ".%03d");
    merge_runs(0, file_count + 1, outfile, run_file_pattern, run_len);
    free(file_name);
    free(run_file_pattern);
    free(index);
    free(heap);
    free(buffer);
}

int compare_func(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

void heapify(int *heap, int n)
{
    int i = n / 2;
    for (; i >= 0; --i)
    {
        sift(heap, i, n);
    }
}

void sift(int *heap, int i, int n)
{
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int smallest = i;
    if (left < n && heap[left] < heap[i])
    {
        smallest = left;
    }
    if (right < n && heap[right] < heap[smallest])
    {
        smallest = right;
    }
    if (smallest != i)
    {
        int temp = heap[i];
        heap[i] = heap[smallest];
        heap[smallest] = temp;
        sift(heap, smallest, n);
    }
}