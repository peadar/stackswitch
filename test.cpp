#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include "stacker.h"

using namespace std;

pthread_attr_t smallStack;

// A recursive and inefficent function to find items in the fibonacci sequence.
uint64_t
fib(uint64_t n)
{
    char buf[1024]; // pad out that stack.
    (void)buf;
    if (n == 0 || n == 1)
        return 1;
    return fib(n - 1) + fib(n - 2);
}

// show the first 20 items of the fibonacci sequence.
void *
showFibs(void *)
{
    for (int i = 0; i < 20; i++)
        cerr << "fibonacci number " << i << " is " << fib(i) << std::endl;
    return 0;
}

// A runnable that prints a member of the fibonacci sequence
class FibRunner : public Runnable {
    uint64_t n;
public:
    FibRunner(uint64_t n_) : n(n_) {}
    public: void run() const {
        cerr << "fibonacci number " << n << " is " << fib(n) << std::endl;
    }
};

// show the first 20 items of the fibonacci sequence, via a stack jumper.
void *
indirectShowFibs(void *arg)
{
    StackSwitch s(1024 * 1024); // big stack.
    for (int i = 0; i < 20; i++)
        s.invoke(FibRunner(i));
    return 0;
}

// Create a thread with a very small stack, and use it to print the first 20 fibonacci numbers, either directly or indirectly.
int
main(int argc, char *argv[])
{
    void *(*how)(void *) = 0;
    int c;
    while ((c = getopt(argc, argv, "di")) != -1) {
        switch (c) {
            case 'd': how = showFibs; break;
            case 'i': how = indirectShowFibs; break;
            default: cerr << "invalid option\n"; exit(1);
        }
    }
    if (how == 0) { 
        cerr << "must give -d or -i\n"; 
        exit(1);
    }

    try {
        PTCALL(pthread_attr_init, (&smallStack));
        PTCALL(pthread_attr_setstacksize, (&smallStack, 16384));
        pthread_t thread;
        PTCALL(pthread_create, (&thread, &smallStack, how, 0));
        void *rv;
        PTCALL(pthread_join, (thread, &rv));
    }
    catch (const exception &err) {
        cerr << err.what() << "\n";
    }
}
