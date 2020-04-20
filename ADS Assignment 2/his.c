#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {false,true} bool;
typedef enum {ADD,DEL,FIND,END,CONTINUE} command_type;
typedef enum {FIRST,BEST,WORST} avail_order;

typedef struct {
        command_type type;
        int key;
        char *record;
        size_t length;
}command;

typedef struct {
        int key;/* Record's key */
        long off; /* Record's offset */
}index_S;

typedef struct {
        int siz;/* Hole's size */
        long off;/* Hole's offset in file */
} avail_S;

#define INDEX   "student.index"
#define AVAILLIST "student.avail"

#define MAX_LENGTH 10000

index_S prim[MAX_LENGTH];
int index_count=0;

avail_S avail[MAX_LENGTH];
int avail_count=0;

void convert_to_command();

void add(FILE *fp,command *com);
void find(FILE *fp,command *com);
void delete(int key);

void delete_index(int del_index);

/* return the available index in available holes*/
int get_available_hole(command *com);
void delete_hole(int avail_index);
void add_new_hole(avail_S newavail);

int compareAscending(const void *l,const void *r);
int compareDescending(const void *l,const void *r);
/*find the target in prim array. return index*/
int binary_search(int target,int low,int high);
/*return true if the record already exists in db file.*/
bool if_record_exist(int key);

void print_index();
void print_avail();

char *buffer;
size_t buffer_length;
size_t buffer_size=1024;

avail_order strategy;

FILE *fp; /* Input/output stream */
FILE *fp_index;
FILE *fp_avail;
long rec_off;
int rec_siz;
char *buf; /* Buffer to hold student record */

int main(int argc,char *argv[]){
        if(argc!=3) {
                printf("Command line arguments are incorrect.\n");
                exit(1);
        }
        if(strcmp(argv[1],"--first-fit")==0) {
                strategy=FIRST;
        }
        else if(strcmp(argv[1],"--best-fit")==0) {
                strategy=BEST;
        }
        else if(strcmp(argv[1],"--worst-fit")==0) {
                strategy=WORST;
        }
        else{
                printf("the second argument is woring, shoule be [--first-fit,--best-fit,--worst-fit]\n");
                return 0;
        }
        char *fname=argv[2];

/* r+ Opens a file to update both reading and writing. The file must exist.
   w+ Creates an empty file for both reading and writing.
 */
        if((fp=fopen(fname,"r+b"))==NULL) {
                fp=fopen(fname,"w+b");
                fp_index=fopen(INDEX,"w+b");
                fp_avail=fopen(AVAILLIST,"w+b");
        }
        else{
                fp_index=fopen(INDEX,"r+b");
                index_S newindex;
                while(fread(&newindex,sizeof(index_S),1,fp_index)==1) {
                        prim[index_count++]=newindex;
                }

                fp_avail=fopen(AVAILLIST,"r+b");
                avail_S newhole;
                while(fread(&newhole,sizeof(avail_S),1,fp_avail)==1) {
                        avail[avail_count++]=newhole;
                }
        }

        while(true) {
                buffer=(char *)malloc(buffer_size*sizeof(char));
                if(buffer==NULL) {
                        printf("Error\n");
                        exit(1);
                }
                buffer_length=getline(&buffer,&buffer_size,stdin);
                /*printf("%zu characters were read.\n",buffer_length);
                   printf("You typed: %s\n",buffer);*/

                command com;
                convert_to_command(buffer,&com);
                if(com.type==END) {
                        fclose(fp);

                        fseek(fp_index,0,SEEK_SET);
                        fwrite(prim,sizeof(index_S),index_count,fp_index);
                        fclose(fp_index);

                        fseek(fp_avail,0,SEEK_SET);
                        fwrite(avail,sizeof(avail_S),avail_count,fp_avail);
                        fclose(fp_avail);
                        break;
                }
                if(com.type==ADD) {
                        add(fp,&com);
                        /*
                           printf("the command is:\n");
                           printf("type: %d\n",com.type);
                           printf("key: %d\n",com.key);
                           printf("record: %s\n",com.record);
                           printf("the record length: %zu\n",com.length);*/
                }
                else if(com.type==FIND) {
                        find(fp,&com);
                }
                else if(com.type==DEL) {
                        delete(com.key);
                }
        }
        print_index();
        print_avail();
        return 0;
}
bool if_record_exist(int key){
        if(binary_search(key,0,index_count-1)==-1) {
                return false;
        }
        return true;
}
/*in ascending order. return index*/
int binary_search(int target,int low,int high){
        int mid;
        while(low<=high) {
                mid=low+(high-low)/2;
                if(prim[mid].key==target) {
                        return mid;
                }
                else if(prim[mid].key<target) {
                        low=mid+1;
                }
                else{
                        high=mid-1;
                }
        }
        return -1;
}

void add(FILE *fp,command *com){
        if(if_record_exist(com->key)) {
                printf("Record with SID=%d exists\n",com->key);
                return;
        }
        long offset;
        int avail_index=get_available_hole(com);
        if(avail_index==-1) {

                fseek(fp,0,SEEK_END);
                offset=ftell(fp);
                fwrite(&(com->length),sizeof(int),1,fp);

                fwrite(com->record,sizeof(char),com->length,fp);
        }
        else{
                offset=avail[avail_index].off;
                fseek(fp,offset,SEEK_SET);
                fwrite(&(com->length),sizeof(int),1,fp);
                fwrite(com->record,sizeof(char),com->length,fp);

                int newholesize=avail[avail_index].siz-sizeof(int)-com->length;
                int newholeoffset=offset+sizeof(int)+com->length;
                delete_hole(avail_index);
                if(newholesize!=0) {
                        avail_S newavail={newholesize,newholeoffset};
                        add_new_hole(newavail);
                }
        }

        /* add index in index file */
        index_S newindex={com->key,offset};
        if(index_count==0) {
                prim[index_count++]=newindex;
        }
        else{
                int low=0,high=index_count-1,mid;
                while(low<=high) {
                        mid=low+(high-low)/2;
                        if(prim[mid].key<com->key) {
                                low=mid+1;
                        }
                        else{
                                high=mid-1;
                        }
                }
                int i=index_count++;
                for(; i>low; --i) {
                        prim[i]=prim[i-1];
                }
                prim[low]=newindex;
        }
}
void find(FILE *fp,command *com){
        int result=binary_search(com->key,0,index_count-1);
        if(result==-1) {
                printf("No record with SID=%d exists\n",com->key);
        }
        else{
                fseek(fp,prim[result].off,SEEK_SET);
                fread(&rec_siz,sizeof(int),1,fp);
                buf=malloc(rec_siz);
                fread(buf,1,rec_siz,fp);
                printf("%s\n",buf);

        }
}

/*delete the specific index, add a hole in available list*/
void delete(int key){
        int del_index=binary_search(key,0,index_count-1);
        if(del_index==-1) {
                printf("No record with SID=%d exists\n",key);
                return;
        }
        avail_S newavail;
        newavail.off=prim[del_index].off;
        fseek(fp,prim[del_index].off,SEEK_SET);
        fread(&rec_siz,sizeof(int),1,fp);
        newavail.siz=sizeof(int)+rec_siz;
        add_new_hole(newavail);

        delete_index(del_index);

}

void delete_index(int del_index){
        if(del_index>=index_count) {
                return;
        }
        int i=del_index;
        for(; i+1<index_count; ++i) {
                prim[i]=prim[i+1];
        }
        index_count--;
}

int get_available_hole(command *com){
        int i=0;
        for(; i<avail_count; ++i) {
                if(avail[i].siz>=(sizeof(int)+com->length)) {
                        return i;
                }
        }
        return -1;
}

void delete_hole(int avail_index){
        if(avail_index>=avail_count) {
                return;
        }
        int i=avail_index;
        for(; i+1<avail_count; ++i) {
                avail[i]=avail[i+1];
        }
        avail_count--;
}

void add_new_hole(avail_S newavail){
        if(strategy==FIRST) {
                avail[avail_count++]=newavail;
        }
        else if(strategy==BEST) {
                avail[avail_count++]=newavail;
                qsort(avail,avail_count,sizeof(avail_S),compareAscending);
        }
        else if(strategy==WORST) {
                avail[avail_count++]=newavail;
                qsort(avail,avail_count,sizeof(avail_S),compareDescending);
        }
}

int compareAscending(const void *l,const void *r){
        avail_S *left=(avail_S*)l;
        avail_S *right=(avail_S*)r;
        if(left->siz==right->siz) {
                return (left->off-right->off);
        }
        else{
                return (left->siz-right->siz);
        }
}

int compareDescending(const void *l,const void *r){
        avail_S *left=(avail_S*)l;
        avail_S *right=(avail_S*)r;
        if(left->siz==right->siz) {
                return (left->off-right->off);
        }
        else{
                return (right->siz - left->siz );
        }
}

void convert_to_command(char *buffer,command *com){
        char buffer_ready[1024];
        strcpy(buffer_ready,buffer);
        free(buffer);

        char *token;
        token=strtok(buffer_ready," \n");
        if(strcmp(token,"end")==0) {
                com->type=END;
        }
        else if(strcmp(token,"add")==0) {
                com->type=ADD;

                token=strtok(NULL," \n");
                com->key=atoi(token);
                token=strtok(NULL," \n");
                com->record=malloc(strlen(token));
                com->length=strlen(token);
                strcpy(com->record,token);

        }
        else if(strcmp(token,"find")==0) {
                com->type=FIND;
                token=strtok(NULL," \n");
                com->key=atoi(token);
        }
        else if(strcmp(token,"del")==0) {
                com->type=DEL;
                token=strtok(NULL," \n");
                com->key=atoi(token);
        }
        else{
                com->type=CONTINUE;
                printf("command should be [add,del,find,end]\n");
        }
}

void print_index(){
  printf("Index:\n");
  int i=0;
  for(;i<index_count;++i){
    printf( "key=%d: offset=%ld\n", prim[i].key, prim[i].off );
  }
}
void print_avail(){
  int hole_siz=0;
  printf("Availability:\n");
  int i=0;
  for(;i<avail_count;++i){
    hole_siz+=avail[i].siz;
    printf( "size=%d: offset=%ld\n", avail[i].siz, avail[i].off );
  }
  printf( "Number of holes: %d\n", avail_count);
  printf( "Hole space: %d\n", hole_siz );


}