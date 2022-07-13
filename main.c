#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#define FOLDER "hash_table/"
#define SUCCESS 0
#define BASE 0
#define ERR_TYPE BASE - 1
#define ERR_SIZE BASE - 2
#define ERR_OPEN_FAIL BASE - 3
#define ERR_LOAD_FAIL BASE - 4
#define ERR_SAVE_FAIL BASE - 5
#define NOT_FOUND BASE - 6
#define BUF_SIZE 12
#define NAME_SIZE 30
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
    int val_size;
    char type;
}dic;

static int hash_BKDR(char*);
static int insert_table(char*, void*, int, char);
static int search_key(dic*);
static int delete_key(char*);
static void find_file(char*, char*);
static void make_folder();
static void generate_data();


int main()
{
    dic data_sample = { .key = "key123456" };
    char* key_sample = data_sample.key;


#ifdef TEST_GENERATE
    make_folder();
    generate_data();
#endif


#ifdef TEST_UPDATE
    char* val = "Bonk";
    printf("key: %s, status(insert): %d\n", key_sample, insert_table(key_sample, val, strlen(val) + 1, 'S'));
#endif


#ifdef TEST_SEARCH
    printf("key: %s, status(search): %d\t", data_sample.key, search_key(&data_sample));

    switch (data_sample.type)
    {
    case 'I':
        printf("val: %d\n", *(int*)data_sample.val);
        break;
    case 'S':
        printf("val: %s\n", data_sample.val);
        break;
    case 'B':
        printf("val: ");
        for (int i = 0; i < data_sample.val_size; i++)
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

static void make_folder()
{
    char* folder_name = malloc(strlen(FOLDER));
    strncpy(folder_name, FOLDER, strlen(FOLDER));
    folder_name[strlen(FOLDER) - 1] = 0;

#ifdef LINUX_SYSTEM
    mkdir(folder_name, S_IRWXU | S_IRWXG | S_IRWXO);    //所有用戶權限全開
#else
    mkdir(folder_name, 777);
#endif

    free(folder_name);
}

static void find_file(char* key, char* file_name)
{
    strncpy(file_name, FOLDER, strlen(FOLDER));

    int index = hash_BKDR(key);

    sprintf(&file_name[strlen(FOLDER)], "%d", index);
    strcat(file_name, ".txt");
}

static int hash_BKDR(char* str)
{
    unsigned int seed = 131;
    unsigned int hash = 0;

    while (*str)
        hash = hash * seed + (*str++);

    return (hash & 0x7fffffff) % TABLE_SIZE;
}

static void generate_data()
{
    char key[15] = "key";
    char buffer[BUF_SIZE];

#ifndef TEST_GENERATE
    srand(time(NULL));
#endif

    time_t start = time(NULL);

    for (int i = 0; i < DATA_SIZE; i++)
    {
        sprintf(buffer, "%d", i);
        strncat(key, buffer, BUF_SIZE);

#ifdef DEBUG
        printf("%d\n", i);
        printf("key:%s, strlen(key):%zu, ", key, strlen(key));
        printf("key[strlen(key)]: %d\n", key[strlen(key)]);
#endif

        int val = rand();
        insert_table(key, &val, sizeof(int), 'I');
        memset(&key[3], 0, 12);
    }

    time_t end = time(NULL);
    printf("time = %f\n", difftime(end, start));
}

static int insert_table(char* key, void* val, int val_size, char type)
{
    switch (type)
    {
    case 'I':
        if (val_size != 4)
            return ERR_SIZE;
        break;
    case 'S':
        if (val_size < 2)
            return ERR_SIZE;
        break;
    case 'B':
        if (val_size < 1)
            return ERR_SIZE;
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

        if (fread(file_buf, fsize, 1, fptr) != 1)
            return ERR_LOAD_FAIL;
        rewind(fptr);

        char* curr_ptr = file_buf;
        int target_loc = 0;
        int val_size_temp = 0;

#ifdef DEBUG
        printf("fsize: %d, file_buf: %p\n", fsize, file_buf);
#endif

        while (curr_ptr < file_buf + fsize)
        {
#ifdef DEBUG
            printf("curr_ptr: %p\n", curr_ptr);
#endif
            if (strcmp(curr_ptr, key) == 0)
            {
                curr_ptr += strlen(curr_ptr) + 1;
                val_size_temp = *(int*)(curr_ptr);
                if (val_size_temp <= 0)
                    return ERR_SIZE;

                target_loc += curr_ptr - file_buf;   // val_len 位置
                curr_ptr += sizeof(int) + sizeof(char) + val_size_temp;
                int fsize_new = fsize - val_size_temp + val_size;
                char* file_buf_new = (char*)malloc(fsize_new);

                memmove(file_buf_new, file_buf, target_loc);

                *(int*)(file_buf_new + target_loc) = val_size;

                switch (type)
                {
                case 'I':   *(file_buf_new + target_loc + sizeof(int)) = 'I';  break;
                case 'S':   *(file_buf_new + target_loc + sizeof(int)) = 'S';  break;
                case 'B':   *(file_buf_new + target_loc + sizeof(int)) = 'B';  break;
                default:    return ERR_TYPE;
                }

                memmove(file_buf_new + target_loc + sizeof(int) + sizeof(char), val, val_size);     //新value
                memmove(file_buf_new + target_loc + sizeof(int) + sizeof(char) + val_size, curr_ptr,
                    fsize - (int)(curr_ptr - file_buf));    //從該value後方到最後

                fclose(fptr);

                fptr = fopen(file_name, "w+");
                if (!fptr)
                    return ERR_OPEN_FAIL;
                if (fwrite(file_buf_new, fsize_new, 1, fptr) != 1)
                    return ERR_SAVE_FAIL;
                fclose(fptr);

                free(file_buf_new);
                free(file_buf);

                return SUCCESS;
            }
            else
            {
                curr_ptr += strlen(curr_ptr) + 1;
                val_size_temp = *(int*)(curr_ptr);
                curr_ptr += sizeof(int) + sizeof(char) + val_size_temp + 1;
            }
        }
        free(file_buf);
    }

    if (fwrite(key, strlen(key) + 1, 1, fptr) != 1)
        return ERR_SAVE_FAIL;

    if (fwrite(&val_size, 4, 1, fptr) != 1)
        return ERR_SAVE_FAIL;

    switch (type)
    {
    case 'I':
        fprintf(fptr, "%c", 'I');
        if (fwrite(val, sizeof(int), 1, fptr) != 1)
            return ERR_SAVE_FAIL;
        break;
    case 'S':
        fprintf(fptr, "%c", 'S');
        if (fwrite(val, val_size, 1, fptr) != 1)
            return ERR_SAVE_FAIL;
        break;
    case 'B':
        fprintf(fptr, "%c", 'B');
        if (fwrite(val, val_size, 1, fptr) != 1)
            return ERR_SAVE_FAIL;
        break;
    default:
        return ERR_TYPE;
    }

#ifdef LINUX_SYSTEM
    fprintf(fptr, "%c", '\n');
#else
    fprintf(fptr, "%c", '\t');
#endif

    fclose(fptr);
    return SUCCESS;
}

static int search_key(dic* data)
{
    char file_name[NAME_SIZE];
    find_file(data->key, file_name);
    FILE* fptr = fopen(file_name, "r");
    if (fptr)
    {
        struct stat fstat;
        stat(file_name, &fstat);
        int fsize = fstat.st_size;
        if (fsize == 0)
            return NOT_FOUND;

        char* file_buf = malloc(fsize);

        if (fread(file_buf, fsize, 1, fptr) != 1)
            return ERR_LOAD_FAIL;
        rewind(fptr);

        char* curr_ptr = file_buf;
        int val_size_temp = 0;

        while (curr_ptr < file_buf + fsize)
        {
            if (strcmp(curr_ptr, data->key) == 0)
            {
                curr_ptr += strlen(curr_ptr) + 1;
                data->val_size = *(int*)(curr_ptr);
                curr_ptr += sizeof(int);
                data->type = *curr_ptr++;
                memmove(data->val, curr_ptr, data->val_size);
                return SUCCESS;
            }
            else
            {
                curr_ptr += strlen(curr_ptr) + 1;
                val_size_temp = *(int*)(curr_ptr);
                curr_ptr += sizeof(int) + sizeof(char) + val_size_temp + 1;
            }
        }
        free(file_buf);
        fclose(fptr);
    }

    return NOT_FOUND;
}

static int delete_key(char* key)
{
    char file_name[NAME_SIZE];
    find_file(key, file_name);
    FILE* fptr = fopen(file_name, "r");
    if (!fptr)
        return ERR_OPEN_FAIL;

    struct stat fstat;
    stat(file_name, &fstat);
    int fsize = fstat.st_size;
    if (fsize == 0)
        return NOT_FOUND;

    char* file_buf = malloc(fsize);
    if (fread(file_buf, fsize, 1, fptr) != 1)
        return ERR_LOAD_FAIL;
    rewind(fptr);

    char* curr_ptr = file_buf;
    int val_size_temp = 0;

    while (curr_ptr < file_buf + fsize)
    {
#ifdef DEBUG
        printf("curr_ptr: %p\n", curr_ptr);
#endif
        if (strcmp(curr_ptr, key) == 0)
        {
            int target_loc = curr_ptr - file_buf;   //紀錄目標 key 開頭位置
            curr_ptr += strlen(curr_ptr) + 1;
            val_size_temp = *(int*)(curr_ptr);
            if (val_size_temp <= 0)
                return ERR_SIZE;

            curr_ptr += sizeof(int) + sizeof(char) + val_size_temp + 1;
            int target_size = curr_ptr - file_buf - target_loc;

            int fsize_new = fsize - target_size;
            char* file_buf_new = (char*)malloc(fsize_new);
            memmove(file_buf_new, file_buf, target_loc);    //從頭到該 key 的 val_len
            memmove(file_buf_new + target_loc, curr_ptr, fsize_new - target_loc);
            fclose(fptr);

            fptr = fopen(file_name, "w+");
            if (!fptr)
                return ERR_OPEN_FAIL;
            if (fwrite(file_buf_new, fsize_new, 1, fptr) != 1)
                return ERR_SAVE_FAIL;
            fclose(fptr);
            free(file_buf_new);
            free(file_buf);

            return SUCCESS;
        }
        else
        {
            curr_ptr += strlen(curr_ptr) + 1;
            val_size_temp = *(int*)(curr_ptr);
            curr_ptr += sizeof(int) + sizeof(char) + val_size_temp + 1;
        }
    }
    free(file_buf);

    return NOT_FOUND;
}

