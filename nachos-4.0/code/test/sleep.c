#include "syscall.h"
main() {
    int i;
    for(i = 0; i < 5; i++) {
        Sleep(1000);
        PrintInt(i);
    }
    return 0;
}