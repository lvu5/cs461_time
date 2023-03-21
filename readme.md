Successfully built take input from user
In xencons_ring.c, I modified the consoleHandler function and xencons_rx to take input from user.
xencons_rx is to print out the input to the console.
In startKernel.c, I added
while(1){
    __cli();
    // in here I do string processing and call the system calls
    __sti();
    HYPERVISOR_sched_op(SCHEDOP_block, 0);
}
In timer.c, I added a pointer to get the future time for the timer, I will check if that time is 
smaller than the current time, if it is, I will call the scheduler. After that I set it to NULL.

To run the code, I used the following command:
make; sudo xl create -c nanoOS.pv