#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#define FOLDER "hash_table"
#define SUCCESS 0
#define BASE 0
#define ERR_TYPE BASE - 1
#define ERR_TYPE_SIZE BASE - 2
#define ERR_NAME_SIZE BASE - 3
#define ERR_MAKE_FAIL BASE - 4
#define ERR_OPEN_FAIL BASE - 5
#define ERR_LOAD_FAIL BASE - 6
#define ERR_SAVE_FAIL BASE - 7
#define ERR_MALLOC_FAIL BASE - 8
#define NOT_FOUND BASE - 9
#define BUF_SIZE 12
#define NAME_SIZE (strlen(FOLDER) + 20)
#define KEY_MAX_SIZE 255
#define VAL_MAX_SIZE 4096
#define DATA_SIZE 10000000
#define TABLE_SIZE ((1 << 13) - 1)
#define LINUX_SYSTEM
#define DEBUG_
#define TEST_GENERATE
#define TEST_UPDATE_
#define TEST_SEARCH_
#define TEST_DELETE_


typedef struct dic
{
    char key[KEY_MAX_SIZE];
    char val[VAL_MAX_SIZE];
    short val_size;
    char type;
}dic;

static int hash_BKDR(char*);
static int insert_table(char*, void*, short, char);
static int search_key(dic*);
static int delete_key(char*);
static int generate_data();
static void find_file(char*, char*);
static int overwrite(char*, int, char*);
static char* find_key(char*, char*, int, FILE*);


int main()
{
    dic data_sample = { .key = "key136" };
    char* key_sample = data_sample.key;


#ifdef TEST_GENERATE
    return generate_data();
#endif


#ifdef TEST_UPDATE
    char* val = "Bonk";
    printf("key: %s, status(insert): %d\n", key_sample, insert_table(key_sample, val, strlen(val) + 1, 'S'));
#endif


#ifdef TEST_SEARCH
    printf("key: %s, status(search): %d", data_sample.key, search_key(&data_sample));

    switch (data_sample.type)
    {
    case 'I':
        printf(",\tval: %d\n", *(short*)data_sample.val);
        break;
    case 'S':
        printf(",\tval: %s\n", data_sample.val);
        break;
    case 'B':
        printf(",\tval: ");
        for (short i = 0; i < data_sample.val_size; i++)
            printf("%c", data_sample.val[i]);
        printf("\n");
        break;
    default:
        return ERR_TYPE;
    }
#endif


#ifdef TEST_DELETE
    printf("key: %s, status(delete): %d\n", data_sample.key, delete_key(key_sample));
    printf("key: %s, status(search): %d\n", data_sample.key, search_key(&data_sample));
#endif


    return 0;
}

static int hash_BKDR(char* str)
{
    unsigned int seed = 131;
    unsigned int hash = 0;

    while (*str)
        hash = hash * seed + (*str++);

    return (hash & 0x7fffffff) % TABLE_SIZE;
}

static int generate_data()
{
    if (mkdir(FOLDER, S_IRWXU | S_IRWXG | S_IRWXO))    //所有用戶權限全開
        return errno;

    char key[15] = "key";
    int modify_loc = strlen(key);

#ifndef TEST_GENERATE
    srand(time(NULL));
#endif

    clock_t start = clock();

    int scale = DATA_SIZE > 10 ? DATA_SIZE / 10 : 1;
    printf("Complete:\n");

    for (int i = 1; i <= DATA_SIZE; i++)
    {
        if (i % scale == 0)
            printf("%d%%\n", i / scale * 10);

        sprintf(&key[modify_loc], "%d", i);

        int val = rand();
        insert_table(key, &val, sizeof(short), 'I');
        memset(&key[modify_loc], 0, 12);
    }

    printf("\n"); 

    clock_t end = clock();
    printf("Time cost: %lf sec\n", (end - start)/(double)(CLOCKS_PER_SEC));

    return SUCCESS;
}

static void find_file(char* key, char* file_name)
{
    int index = hash_BKDR(key);
    sprintf(file_name, "%s/%d%s", FOLDER, index, ".txt");
}

static int insert_table(char* key, void* val, short val_size, char type)
{
    switch (type)
    {
    case 'I':
        if (val_size != 2)
            return ERR_TYPE_SIZE;
        break;
    case 'S':
        if (val_size < 2)
            return ERR_TYPE_SIZE;
        break;
    case 'B':
        if (val_size < 1)
            return ERR_TYPE_SIZE;
        break;
    default:
        return ERR_TYPE;
    }

    char file_name[NAME_SIZE];
    find_file(key, file_name);

    FILE* fptr = fopen(file_name, "a+");
    if (!fptr)
        return ERR_OPEN_FAIL;
	
	struct stat fstat;
    stat(file_name, &fstat);
    int fsize = fstat.st_size;

    if (fsize)
    {
        char* file_buf = malloc(fsize);
        if (file_buf == NULL)
            return ERR_MALLOC_FAIL;
        if (fread(file_buf, fsize, 1, fptr) != 1)
        {
            free(file_buf);
            return ERR_LOAD_FAIL;
        }

        char* target_ptr = find_key(key, file_buf, fsize, fptr);
        if (target_ptr)
        {
            fclose(fptr);

            target_ptr += strlen(target_ptr) + 1;
            short val_size_temp = *(short*)(target_ptr);

            int fsize_new = fsize - val_size_temp + val_size;
            char* file_buf_new = (char*)malloc(fsize_new);

            char* target_ptr_new = file_buf_new;

            int target_loc = target_ptr - file_buf;
            memmove(target_ptr_new, file_buf, target_loc);

            target_ptr_new += target_loc;
            *(short*)(target_ptr_new) = val_size;

            target_ptr_new += sizeof(short);
            *target_ptr_new = type;

            target_ptr_new += sizeof(char);
            memmove(target_ptr_new, val, val_size);

            target_ptr_new += val_size;
            target_ptr += sizeof(short) + sizeof(char) + val_size_temp;
            memmove(target_ptr_new, target_ptr, 
                    fsize_new - (int)(target_ptr_new - file_buf_new));

            free(file_buf);
            int error = overwrite(file_buf_new, fsize_new, file_name);
            free(file_buf_new);
            if(error)
                return error;

            return SUCCESS;
        }
        free(file_buf);
    }

    if (fwrite(key, strlen(key) + 1, 1, fptr) != 1)
        return ERR_SAVE_FAIL;

    if (fwrite(&val_size, sizeof(short), 1, fptr) != 1)
        return ERR_SAVE_FAIL;

    if (fprintf(fptr, "%c", type) < 0)
        return errno;

    if (fwrite(val, val_size, 1, fptr) != 1)
        return ERR_SAVE_FAIL;

#ifdef LINUX_SYSTEM
    fprintf(fptr, "%c", '\n');
#else
    fprintf(fptr, "%c", '\t');
#endif

    fclose(fptr);

    return SUCCESS;
}

static int overwrite(char* file_buf_new, int fsize_new, char* file_name)
{
    char file_name_temp[NAME_SIZE];
    sprintf(file_name_temp, "%s/temp%s", FOLDER, ".txt");
    FILE* fptr = fopen(file_name_temp, "w+");
    if (!fptr)
        return ERR_OPEN_FAIL;

    if (fwrite(file_buf_new, fsize_new, 1, fptr) != 1)
        return ERR_SAVE_FAIL;

    if (rename(file_name_temp, file_name))
        return errno;

    fclose(fptr);
    return SUCCESS;
}

static char* find_key(char* key, char* file_buf, int fsize, FILE* fptr)
{
    char* curr_ptr = file_buf;

    while (curr_ptr < file_buf + fsize)
    {
        if (strcmp(curr_ptr, key) == 0)
            return curr_ptr;
        else
        {
            curr_ptr += strlen(curr_ptr) + 1;
            curr_ptr += sizeof(short) + sizeof(char) + *(short*)(curr_ptr) + 1;
        }
    }
    return NULL;
}

static int search_key(dic* data)
{
    char file_name[NAME_SIZE];
    find_file(data->key, file_name);

    struct stat fstat;
    stat(file_name, &fstat);
    int fsize = fstat.st_size;
    if (fsize <= 0)
        return NOT_FOUND;

    FILE* fptr = fopen(file_name, "r");
    if (!fptr)
        return ERR_OPEN_FAIL;

    char* file_buf = malloc(fsize);
    if (file_buf == NULL)
        return ERR_MALLOC_FAIL;
    if (fread(file_buf, fsize, 1, fptr) != 1)
    {
        free(file_buf);
        return ERR_LOAD_FAIL;
    }

    char* target_ptr = find_key(data->key, file_buf, fsize, fptr);
    if (target_ptr)
    {
        target_ptr += strlen(target_ptr) + 1;
        data->val_size = *(short*)(target_ptr);
        target_ptr += sizeof(short);
        data->type = *target_ptr++;
        memmove(data->val, target_ptr, data->val_size);
        free(file_buf);

        return SUCCESS;
    }
    free(file_buf);

    return NOT_FOUND;
}

static int delete_key(char* key)
{
    char file_name[NAME_SIZE];
    find_file(key, file_name);

    struct stat fstat;
    stat(file_name, &fstat);
    int fsize = fstat.st_size;
    if (fsize <= 0)
        return NOT_FOUND;

    FILE* fptr = fopen(file_name, "r");
    if (!fptr)
        return ERR_OPEN_FAIL;

    char* file_buf = malloc(fsize);
    if (file_buf == NULL)
        return ERR_MALLOC_FAIL;
    if (fread(file_buf, fsize, 1, fptr) != 1)
    {
        free(file_buf);
        return ERR_LOAD_FAIL;
    }

    char* target_ptr = find_key(key, file_buf, fsize, fptr);
    if (target_ptr)
    {
        fclose(fptr);
        
        int target_loc = target_ptr - file_buf;
        target_ptr += strlen(target_ptr) + 1;
        short val_size_temp = *(short*)(target_ptr);
        if (val_size_temp <= 0)
        {
            free(file_buf);
            return ERR_TYPE_SIZE;
        }

        target_ptr += sizeof(short) + sizeof(char) + val_size_temp + 1;
        int target_size = target_ptr - (file_buf + target_loc);

        int fsize_new = fsize - target_size;
        char* file_buf_new = (char*)malloc(fsize_new);
        if (file_buf_new == NULL)
        {
            free(file_buf);
            return ERR_TYPE_SIZE;
        }

        memmove(file_buf_new, file_buf, target_loc);    //從頭到該 key 的 val_len
        memmove(file_buf_new + target_loc, target_ptr, fsize_new - target_loc);

        free(file_buf);
        int error = overwrite(file_buf_new, fsize_new, file_name);
        free(file_buf_new);
        if(error)
            return error;
        
        return SUCCESS;
    }

    free(file_buf);

    return NOT_FOUND;
}

