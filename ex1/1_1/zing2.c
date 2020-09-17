#include <stdio.h>
#include <unistd.h>

void zing ()
{ 
    char *name;
    name = getlogin();
    printf("%s is the best team\n", name);
}






