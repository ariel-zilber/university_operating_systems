//
// Created by arikzil on 17/04/2020.
//

#include <iostream>
#include "Thread.h"


#ifdef __x86_64__
/* code for 64 bit Intel arch */
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
        "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}
#endif


int Thread::getPriority() const
{
    return _priority;
}

void Thread::setPriority(int priority)
{
    _priority = priority;
}


int Thread::getTid() const
{
    return _tid;
}


STATES Thread::getState() const
{
    return _state;
}

void Thread::setState(STATES state)
{
    _state = state;
}

int Thread::getQuantumCount() const
{
    return _quantumCount;
}


Thread::Thread(int priority, STATES state, void (*f)(), int tid) :
        _priority(priority),
        _state(state),
        _f(f),
        _tid(tid),
        _quantumCount(0)
{

    _stack = new char[STACK_SIZE];

    address_t sp, pc;
    sp = (address_t) _stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t) _f;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);

}

void Thread::loadEnvironment()
{

    siglongjmp(env, 1);
}

bool Thread::saveEnvironment()
{
    int retVal = sigsetjmp(env, 1);

    return retVal == 0;
}

void Thread::increaseQuantum()
{
    _quantumCount++;
}

Thread::~Thread()
{
    delete[]_stack;
}
