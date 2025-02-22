#include "kernel/types.h"
#include "user/user.h"


uint64 pa;

int
routine()
{
    int stack_value_2;

    void *ptr = (void *)&stack_value_2;
    pa = getpaddr(ptr);
    printf("stack_value_2 va: %p ==> pa: %lx\n", ptr, pa);

    
    return 0;
}

int
main(int argc, char *argv[])
{
    
    printf("my id: %d\n", getpid());


    //test 1: system call getppid    
    
    printf("my parent: %d\n", getppid());



    //test 2: system call getcpids

    for(int i=0; i<10; i++){
        int ret = fork();
        if(ret==0){
            sleep(10);
            exit(0);
        }else if(ret>0){
            printf("create a child: %d\n", ret);
        }else{
            printf("something goes wrong!");
            exit(1);
        } 
    }

    int cpids[10];
    int nchild = getcpids(cpids,20);

    printf("nchild=%d\n", nchild);
    for(int i=0; i<nchild; i++) printf("child #%d: %d\n", i, cpids[i]);

    for(int i=0; i<10; i++) wait(0);




    //testing 3: system call getpaddr
    
    void *ptr = (void *)main;
    pa = getpaddr(ptr);
    printf("code segment va: %p ==> pa: %lx\n", ptr, pa);

    ptr = (void *)&pa;
    pa = getpaddr(ptr);
    printf("static_value va: %p ==> pa: %lx\n", ptr, pa);

    int stack_value_1;
    ptr = (void *)&stack_value_1;
    pa = getpaddr(ptr);
    printf("stack_value_1 va: %p ==> pa: %lx\n", ptr, pa);

    routine();
    routine();

    ptr = sbrk(4096);
    pa = getpaddr(ptr);
    printf("heap_block_1 va: %p ==> pa: %lx\n", ptr, pa);

    ptr = sbrk(4096);
    pa = getpaddr(ptr);
    printf("heap_block_2 va: %p ==> pa: %lx\n", ptr, pa);



    //test 4: system call gettraphistory

    double x=0.0;
    for(int i=0; i<100000; i++)
        for(int j=0; j<100000; j++)
            x+=(i*100000.0+j*1.0)*(j*100000.0+i*1.0);
    
    int trapcount, syscallcount, devintcount, timerintcount;
    gettraphistory(&trapcount, &syscallcount, &devintcount, &timerintcount);
    printf("trapcount=%d, syscallcount=%d, devintcount=%d, timerintcount=%d\n",
        trapcount,syscallcount,devintcount,timerintcount
    );


}
