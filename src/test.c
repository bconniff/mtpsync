#include "test/list_test.h"
#include "test/hash_test.h"
#include "test/fs_test.h"
#include "test/str_test.h"
#include "test/sync_test.h"
#include "test/args_test.h"

int main(int argc, char **argv) {
    hash_test(1);
    hash_test(8);
    list_test(1);
    list_test(8);
    fs_test();
    str_test();
    sync_test();
    args_test();
}
