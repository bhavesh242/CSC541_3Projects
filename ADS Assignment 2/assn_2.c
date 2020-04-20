#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLEN 10000
typedef struct
{
    int key;
    long off;
} index_S;

typedef struct
{
    int siz;
    long off;
} avail_S;
index_S indexes[MAXLEN];
avail_S avails[MAXLEN];
int indexCount;
int availCount;
int method;
void readIndexes();
void readAvailables();
int countIndex();
int countAvail();
char **parseInput(char *line);
void add(FILE *fp, int primary_key, char *record);
void find(FILE *fp, int key);
void delete (FILE *fp, int key);
int binary_search(int key, int left, int right);
int find_available_hole(int size);
int comparator_best_fit(const void *v1, const void *v2);
int comparator_worst_fit(const void *v1, const void *v2);
void remove_hole(int holeIndex);
void add_hole(int residual_hole_size, long residual_hole_offset);
void add_index(int key, long offset);
void remove_index(int key);
void write_index_file();
void write_holes_file();
void print_indexes();
void print_holes_size_and_count();
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Improper Number of Arguments Given, Give Arguments as follows : assn_2 avail-list-order studentfile-name");
        return 0;
    }
    if (strcmp(argv[1], "--first-fit") == 0)
    {
        method = 1;
    }
    else if (strcmp(argv[1], "--best-fit") == 0)
    {
        method = 2;
    }
    else if (strcmp(argv[1], "--worst-fit") == 0)
    {
        method = 3;
    }
    else
    {
        printf("Improper Availibility List method Specified, please select amongst : --first-fit, --best-fit, --worst-fit");
        return 0;
    }
    FILE *fp;
    if ((fp = fopen(argv[2], "r+b")) == NULL)
    {
        fp = fopen(argv[2], "w+b");
        indexCount = 0;
        availCount = 0;
    }
    else
    {
        readIndexes();
        readAvailables();
    }
    while (1)
    {
        char line[200];
        fgets(line, 200, stdin);
        char **instr = parseInput(line);
        if (strcmp(instr[0], "add") == 0)
        {
            char **records = parseInput(instr[1]);
            int primary_key = atoi(records[0]);
            add(fp, primary_key, records[1]);
        }
        else if (strcmp(instr[0], "find") == 0)
        {
            find(fp, atoi(instr[1]));
        }
        else if (strcmp(instr[0], "del") == 0)
        {
            delete (fp, atoi(instr[1]));
        }
        else if (strcmp(instr[0], "end") == 0)
        {
            break;
        }
        else
        {
            continue;
        }
    }
    fclose(fp);
    write_index_file();
    write_holes_file();
    print_indexes();
    print_holes_size_and_count();
    return 0;
}
void readIndexes()
{
    indexCount=0;
    FILE *findex = fopen("index.bin", "r+b");
    index_S record;
    while (fread(&record, sizeof(index_S), 1, findex) == 1)
    {
        indexes[indexCount++] = record;
    }
    fclose(findex);
}
void readAvailables()
{
    availCount=0;
    FILE *favail = fopen("avail.bin", "r+b");
    avail_S hole;
    while (fread(&hole, sizeof(avail_S), 1, favail) == 1)
    {
        avails[availCount++] = hole;
    }
    fclose(favail);
}
void add(FILE *fp, int primary_key, char *record)
{
    int holeIndex, record_size, residual_hole_size;
    long residual_hole_offset;
    int index = binary_search(primary_key, 0, indexCount - 1);
    long offset;
    if (index != -1)
    {
        printf("Record with SID=%d exists\n", primary_key);
        return;
    }
    holeIndex = find_available_hole(strlen(record));
    record_size = strlen(record);
    if (holeIndex == -1)
    {
        fseek(fp, 0, SEEK_END);
        offset = (long)ftell(fp);
        fwrite(&record_size, sizeof(int), 1, fp);
        fwrite(record, sizeof(char), record_size, fp);
    }
    else
    {
        offset = avails[holeIndex].off;
        fseek(fp, offset, SEEK_SET);
        fwrite(&record_size, sizeof(int), 1, fp);
        fwrite(record, sizeof(char), record_size, fp);
        residual_hole_size = avails[holeIndex].siz - (sizeof(int) + record_size);
        residual_hole_offset = offset + (sizeof(int) + record_size);
        remove_hole(holeIndex);
        add_hole(residual_hole_size, residual_hole_offset);
    }
    add_index(primary_key, offset);
}
void find(FILE *fp, int key)
{
    int record_size;
    char *record = NULL;
    int index = binary_search(key, 0, indexCount - 1);
    if (index == -1)
    {
        printf("No record with SID=%d exists\n", key);
        return;
    }
    fseek(fp, indexes[index].off, SEEK_SET);
    fread(&record_size, sizeof(int), 1, fp);
    record = (char *)malloc(record_size * sizeof(char));
    fread(record, 1, record_size, fp);
    printf("%s\n", record);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
}
void delete (FILE *fp, int key)
{
    int index = binary_search(key, 0, indexCount - 1);
    long offset;
    int size;
    if (index == -1)
    {
        printf("No record with SID=%d exists\n", key);
        return;
    }
    offset = indexes[index].off;
    fseek(fp, indexes[index].off, SEEK_SET);
    fread(&size, sizeof(int), 1, fp);
    size = (sizeof(int) + size);
    add_hole(size, offset);
    remove_index(index);
}
int binary_search(int key, int left, int right)
{
    int mid;
    while (left <= right)
    {
        mid = (left + right) / 2;
        if (key == indexes[mid].key)
        {
            return mid;
        }
        else if (key > indexes[mid].key)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }
    return -1;
}
char **parseInput(char *line)
{
    char **inst = (char **)malloc(sizeof(char *) * 2);
    char *ptr = strtok(line, " \n");
    inst[0] = ptr;
    ptr = strtok(NULL, "\n");
    inst[1] = ptr;
    return inst;
}
int find_available_hole(int size)
{
    int i = 0;
    while (i < availCount)
    {
        if (avails[i].siz >= (size + sizeof(int)))
        {
            return i;
        }
        i++;
    }
    return -1;
}
void remove_hole(int holeIndex)
{
    int i;
    if (holeIndex > availCount - 1)
    {
        return;
    }
    for ( i = holeIndex; i < availCount - 1; i++)
    {
        avails[i] = avails[i + 1];
    }
    availCount = availCount - 1;
}
void add_hole(int hole_size, long hole_offset)
{
    if (hole_size <= 0)
    {
        return;
    }
    avail_S hole = {hole_size, hole_offset};
    avails[availCount++] = hole;
    if (method == 1)
    {
        return;
    }
    else if (method == 2)
    {
        qsort(avails, availCount, sizeof(avail_S), comparator_best_fit);
    }
    else if (method == 3)
    {
        qsort(avails, availCount, sizeof(avail_S), comparator_worst_fit);
    }
}
int comparator_best_fit(const void *v1, const void *v2)
{
    int size1 = ((avail_S *)v1)->siz;
    int size2 = ((avail_S *)v2)->siz;
    if( size1!=size2)
    {
        return size1 - size2;
    }
    return ((avail_S *)v1)->off - ((avail_S *)v2)->off; 
}
int comparator_worst_fit(const void *v1, const void *v2)
{
    int size1 = ((avail_S *)v1)->siz;
    int size2 = ((avail_S *)v2)->siz;
    if( size1!=size2)
    {
        return size2 - size1;
    }
    return ((avail_S *)v1)->off - ((avail_S *)v2)->off; 
}
void add_index(int key, long offset)
{
    int left, right, mid,i;
    index_S index = {key, offset};
    if (indexCount == 0)
    {
        indexes[indexCount++] = index;
        return;
    }
    left = 0;
    right = indexCount - 1;
    while (left <= right)
    {
        mid = (left + right) / 2;
        if (key > indexes[mid].key)
        {
            left = mid + 1;
        }
        else if (key < indexes[mid].key)
        {
            right = mid - 1;
        }
    }
    for (i = indexCount; i > left; i--)
    {
        indexes[i] = indexes[i - 1];
    }
    indexes[left] = index;
    indexCount++;
}
void remove_index(int index)
{
    int i;
    if (index > indexCount - 1)
    {
        return;
    }
    for (i = index; i < indexCount - 1; i++)
    {
        indexes[i] = indexes[i + 1];
    }
    indexCount = indexCount - 1;
}
void write_index_file()
{
    FILE *findex = fopen("index.bin", "w+b");
    fseek(findex, 0, SEEK_SET);
    fwrite(indexes, sizeof(index_S), indexCount, findex);
    fclose(findex);
}
void write_holes_file()
{
    FILE *favail = fopen("avail.bin", "w+b");
    fseek(favail, 0, SEEK_SET);
    fwrite(avails, sizeof(avail_S), availCount, favail);
    fclose(favail);
}
void print_indexes()
{
    int i;
    printf("Index:\n");
    for (i = 0; i < indexCount; ++i)
    {
        printf("key=%d: offset=%ld\n", indexes[i].key, indexes[i].off);
    }
}
void print_holes_size_and_count()
{
    int i,total_hole_space = 0;
    printf("Availability:\n");
    for (i = 0; i < availCount; i++)
    {
        printf("size=%d: offset=%ld\n", avails[i].siz, avails[i].off);
        total_hole_space = total_hole_space + avails[i].siz;
    }
    printf("Number of holes: %d\n", availCount);
    printf("Hole space: %d\n", total_hole_space);
}