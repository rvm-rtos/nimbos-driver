#include <stdio.h>

#define DEV_PATH  "/dev/nimbos"

int main()
{
    printf("Hello NimbOS!\n");

    FILE* f = fopen(DEV_PATH, "r");
    printf("%p\n", f);
    fclose(f);

    return 0;
}
