#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include "bpt.h"
#include "file.h"

// MAIN
int main( int argc, char ** argv ) {
    int64_t input_key;
    char input_value[SIZE_VALUE];
    char instruction;
    int table_id;

    license_notice();
    usage_1();  
    usage_2();

    init_db(1000);
    table_id = open_table("test.db");
    print_tree(table_id);
    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'i':
            scanf("%" PRIu64 " %s", &input_key, input_value);
            insert(table_id, input_key, input_value);
            print_tree(table_id);
            break;
        case 'd':
            scanf("%" PRIu64 "", &input_key);
            remove(table_id, input_key);
            print_tree(table_id);
            break;
        case 'f':
        case 'p':
            scanf("%" PRIu64 "", &input_key);
            find_and_print(table_id, input_key);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            close_table(table_id);
            shutdown_db();
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(table_id);
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    close_table(table_id);
    shutdown_db();

    return EXIT_SUCCESS;
}
