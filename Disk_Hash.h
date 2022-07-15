#ifndef DISK_HASH_H
#define DISK_HASH_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "Disk_Hash.h"
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
#define MAKE_FOLDER_
#define TEST_GENERATE_
#define TEST_UPDATE_
#define TEST_SEARCH_
#define TEST_DELETE_
#define DELETE_FOLDER


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
static char* key_location(char*, char*, int, FILE*);

#endif

