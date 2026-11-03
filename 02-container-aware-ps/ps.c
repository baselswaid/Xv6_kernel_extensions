#include "types.h"
#include "user.h"

//user-mode interface
int main(){
    printf(1, "Running ps command:\n");
    cps115(); //call the `ps` system call
    exit(0); //exit the program
}

