#include <stdlib.h>
#include "gb.h"

int main()
{
    gb gameboy;

    int ret = gameboy.step();
    if (ret) {
        exit(ret);
    }
}

