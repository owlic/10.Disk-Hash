#include "Disk_Hash.h"
#include "Disk_Hash.c"

int main()
{
    dic data_sample = { .key = "key136" };
    char* key_sample = data_sample.key;


#ifdef MAKE_FOLDER
    if (mkdir(FOLDER, S_IRWXU | S_IRWXG | S_IRWXO))     //所有用戶權限全開
        return errno;
#endif


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


#ifdef DELETE_FOLDER
    return rmrf(FOLDER);
#endif


    return 0;
}

