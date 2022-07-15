#include "Disk_Hash.h"

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

        char* target_ptr = key_location(key, file_buf, fsize, fptr);
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
}

static char* key_location(char* key, char* file_buf, int fsize, FILE* fptr)
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

    char* target_ptr = key_location(data->key, file_buf, fsize, fptr);
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

    char* target_ptr = key_location(key, file_buf, fsize, fptr);
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

#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>
#include <unistd.h>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

