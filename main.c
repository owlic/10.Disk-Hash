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
#define NAME_SIZE 50
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

typedef struct info
{
    char* key;
    char file_name[NAME_SIZE];
    int fsize;
    char* file_buf; //需要 free
    char* target_ptr;
    int error_code;
    FILE* fptr;
}info;


static int hash_BKDR(char*);
static void find_key(info*, bool);
static int insert_table(char*, void*, short, char);
static int search_key(dic*);
static int delete_key(char*);
static int generate_data();


int main()
{
    dic data_sample = { .key = "key2094" };
    char* key_sample = data_sample.key;


#ifdef TEST_GENERATE
    return generate_data();
#endif


#ifdef TEST_UPDATE
    char* val = "Bonk";
    printf("key: %s, status(insert): %d\n", key_sample, insert_table(key_sample, val, strlen(val) + 1, 'S'));
#endif


#ifdef TEST_SEARCH
    printf("key: %s, status(search): %d,\t", data_sample.key, search_key(&data_sample));

    switch (data_sample.type)
    {
    case 'I':
        printf("val: %d\n", *(short*)data_sample.val);
        break;
    case 'S':
        printf("val: %s\n", data_sample.val);
        break;
    case 'B':
        printf("val: ");
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

    int scale = DATA_SIZE / 10;
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
    printf("time = %lf\n", (end - start)/(double)(CLOCKS_PER_SEC));

    return SUCCESS;
}

static void find_key(info* data, bool add)	//search_key(dic* data)
{
    int index = hash_BKDR(data->key);
    sprintf(data->file_name, "%s/%d%s", FOLDER, index, ".txt");
    if (add)
        data->fptr = fopen(data->file_name, "a+");
    else
        data->fptr = fopen(data->file_name, "r");
	
	if (data->fptr)
    {
        struct stat fstat;
        stat(data->file_name, &fstat);
        data->fsize = fstat.st_size;

        if (data->fsize)
        {
            data->file_buf = malloc(data->fsize);
            if (data->file_buf == NULL)
                data->error_code = ERR_MALLOC_FAIL;

            if (fread(data->file_buf, data->fsize, 1, data->fptr) != 1)
                data->error_code = ERR_LOAD_FAIL;

            char* curr_ptr = data->file_buf;

            while (curr_ptr < data->file_buf + data->fsize)
            {
                if (strcmp(curr_ptr, data->key) == 0)
                {
                    data->target_ptr = curr_ptr;
                    return;
			    }
	            else
                {
                    curr_ptr += strlen(curr_ptr) + 1;
                    curr_ptr += sizeof(short) + sizeof(char) + *(short*)(curr_ptr) + 1;
                }
		    }
            free(data->file_buf);
            return;
        }
        else if (add == false)      //查找和刪除
            data->error_code = NOT_FOUND;
    }
    else
        data->error_code = errno;
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

    info data = { .key = key };
    find_key(&data, true);
    if (data.error_code)
        return data.error_code;        

    if (data.target_ptr)
    {
        char* curr_ptr = data.target_ptr;

        curr_ptr += strlen(curr_ptr) + 1;
        short val_size_temp = *(short*)(curr_ptr);
        if (val_size_temp <= 0)
        {
            free(data.file_buf);
            return ERR_TYPE_SIZE;
        }
        int fsize_new = data.fsize - val_size_temp + val_size;
        char* file_buf_new = (char*)malloc(fsize_new);
        if (file_buf_new == NULL)
        {
            free(data.file_buf);
            return ERR_MALLOC_FAIL;
        }
        char* curr_ptr_new = file_buf_new;

        int target_loc = curr_ptr - data.file_buf;   // val_len 開頭位置
        memmove(curr_ptr_new, data.file_buf, target_loc);

        curr_ptr_new += target_loc;
        *(short*)(curr_ptr_new) = val_size;

        curr_ptr_new += sizeof(short);
        *curr_ptr_new = type;

        curr_ptr_new += sizeof(char);
        memmove(curr_ptr_new, val, val_size);     //新value

        curr_ptr_new += val_size;
        curr_ptr += sizeof(short) + sizeof(char) + val_size_temp;
        memmove(curr_ptr_new, curr_ptr, 
                fsize_new - (int)(curr_ptr_new - file_buf_new));    //從該value後方到最後
        fclose(data.fptr);

        char file_name_temp[NAME_SIZE];
        sprintf(file_name_temp, "%s/temp%s", FOLDER, ".txt");

        FILE* fptr = fopen(file_name_temp, "w+");
        if (!fptr)
        {
            free(data.file_buf);
            return ERR_OPEN_FAIL;
        }

        if (fwrite(file_buf_new, fsize_new, 1, fptr) != 1)
        {
            free(data.file_buf);
            return ERR_SAVE_FAIL;
        }

        rename(file_name_temp, data.file_name);

        fclose(fptr);
        free(file_buf_new);
        free(data.file_buf);   //記得

        return SUCCESS;
    }

    if (fwrite(key, strlen(key) + 1, 1, data.fptr) != 1)
        return ERR_SAVE_FAIL;

    if (fwrite(&val_size, sizeof(short), 1, data.fptr) != 1)
        return ERR_SAVE_FAIL;

    if (fprintf(data.fptr, "%c", type) < 0)
        return errno;

    if (fwrite(val, val_size, 1, data.fptr) != 1)
        return ERR_SAVE_FAIL;

#ifdef LINUX_SYSTEM
    fprintf(data.fptr, "%c", '\n');
#else
    fprintf(data.fptr, "%c", '\t');
#endif

    fclose(data.fptr);
    return SUCCESS;
}

static int search_key(dic* request)
{
    info data = { .key = request->key };
    find_key(&data, false);
    if (data.error_code)
        return data.error_code;

    if (data.target_ptr)
    {
        char* curr_ptr = data.target_ptr;
        curr_ptr += strlen(curr_ptr) + 1;
        request->val_size = *(short*)(curr_ptr);
        curr_ptr += sizeof(short);
        request->type = *curr_ptr++;
        memmove(request->val, curr_ptr, request->val_size);
        free(data.file_buf);   //記得
        return SUCCESS;
    }
    else
        return NOT_FOUND;
}

static int delete_key(char* key)
{
    info data = { .key = key };
    find_key(&data, false);

    if (data.target_ptr)
    {
        char* curr_ptr = data.target_ptr;

        int target_loc = curr_ptr - data.file_buf;   //紀錄目標 key 開頭位置
        curr_ptr += strlen(curr_ptr) + 1;
        short val_size_temp = *(short*)(curr_ptr);
        if (val_size_temp <= 0)
        {
            free(data.file_buf);
            return ERR_TYPE_SIZE;
        }

        curr_ptr += sizeof(short) + sizeof(char) + val_size_temp + 1;
        int target_size = curr_ptr - (data.file_buf + target_loc);

        int fsize_new = data.fsize - target_size;
        char* file_buf_new = (char*)malloc(fsize_new);
        if (file_buf_new == NULL)
        {
            free(data.file_buf);
            return ERR_MALLOC_FAIL;
        }
        memmove(file_buf_new, data.file_buf, target_loc);    //從頭到該 key 的 val_len
        memmove(file_buf_new + target_loc, curr_ptr, fsize_new - target_loc);
        fclose(data.fptr);

        char file_name_temp[NAME_SIZE];
        sprintf(file_name_temp, "%s/temp%s", FOLDER, ".txt");

        FILE* fptr = fopen(file_name_temp, "w+");
        if (!fptr)
        {
            free(data.file_buf);
            return ERR_OPEN_FAIL;
        }

        if (fwrite(file_buf_new, fsize_new, 1, fptr) != 1)
        {
            free(data.file_buf);
            return ERR_SAVE_FAIL;
        }

        rename(file_name_temp, data.file_name);

        fclose(fptr);
        free(file_buf_new);
        free(data.file_buf);

        return SUCCESS;
    }
    else
        return NOT_FOUND;
}

