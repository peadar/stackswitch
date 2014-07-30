#include "stacker.h"
#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

extern "C" {
static void *thread_entry(void *);
}

// Create a stacker: create a buffer for its stack, briefly start a thread to set up a
// frame we can jump into
StackSwitch::StackSwitch(size_t size)
    : buf(new char[size])
    , running(0)
{
    pthread_attr_t attributes;
    PTCALL(pthread_attr_init, (&attributes));
    PTCALL(pthread_attr_setstack, (&attributes, (void *)buf, size));
    pthread_t pt;
    PTCALL(pthread_create,(&pt, &attributes, thread_entry, this));
    void *rv;
    PTCALL(pthread_join,(pt, &rv))
    PTCALL(pthread_attr_destroy, (&attributes));

    // The exit of the thread may have screwed up the stack frame. The thread
    // snapshotted the memory sandwiched between two on-stack variables that
    // bound anything important, so we can replace that now.
    memcpy(tosp, scratch, bosp - tosp);
    delete[] scratch;
    scratch = 0;
}

// Trampoline for the thread: just call setstack on the passed stacker.
extern "C" {
void *
thread_entry(void *v)
{
    StackSwitch *s = (StackSwitch *)v;
    char bos;
    s->setstack(&bos);
    return 0;
}
}

// Set up the stack frame, and then wait for something to jump into us.

void
StackSwitch::setstack(char *bosp_)
{
    copystack(bosp_);
    if (setjmp(enter) != 0) {
        running->run();
        longjmp(leave, 1);
    }
}

/*
 * Snapshot the local environment before exiting the live thread: this gives
 * us enough state in "scratch" to copy over the stack that has possibly been
 * ruined by the thread exiting.
 */
void
StackSwitch::copystack(char *bosp_)
{
    bosp = bosp_;
    char tos;
    tosp = &tos;
    if (bosp - tosp < 0)
        swap(bosp, tosp);
    size_t scratchNeed = bosp - tosp;
    scratch =  new char[scratchNeed];
    memcpy(scratch, tosp, bosp - tosp);
}

// Invoke "runnable" on our alternate stack.
void
StackSwitch::invoke(const Runnable &runnable)
{
    assert(running == 0);
    running = &runnable;
    if (setjmp(leave) == 0)
        longjmp(enter, 1);
    running = 0;
}

StackSwitch::~StackSwitch()
{
    delete[] buf;
}
