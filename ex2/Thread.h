#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#include <setjmp.h>
#include <signal.h>
#include "uthreads.h"

/***
 *
 * Represents the different states a thread can be found at
 *
 * */
enum STATES
{
    RUNNING, READY, BLOCKED
};


/**
 *
 * Represents a thread ,per ex2 description.
 *
 * ***/
class Thread
{
public:
    /**
     * @param priority the thread's priority
     * @param state the state the priority should be foind at
     * @param f the function the thread executes
     * @param tid the ID of the thread
     * */
    Thread(int priority, STATES state, void (*f)(), int tid);

    /**
     * Getter to priority property
     *
     * @return the thread's priority
     * **/
    int getPriority() const;

    /**
     * Setter to the priority property of the thread
     * @param priority the updated priorityy of the thread
     * **/
    void setPriority(int priority);


    /**
     * Getter to the tid property
     * @return the tid of the thread
     * **/
    int getTid() const;

    /**
     * Getter to the state property
     * @return the current state of the thread
     * **/
    STATES getState() const;

    /**
     * Setter to the state property of the thread
     * @param state the updated state of the thread
     * **/
    void setState(STATES state);


    /**
     * Getter to the quantum property of  the thread
     * @return the quantum count of
     * **/
    int getQuantumCount() const;


    /**
     *
     * Increases the quantum count of the thread by 1
     *
     *
     * **/
    void increaseQuantum();


    /**
     * Performs "nonlocal gotos" to the content of the jump
     *
     * */
    void loadEnvironment();

    /**
     * Sets the environment to be used in "nonlocal gotos"
     *
     * */
    bool saveEnvironment();


    /**
     * Destructor
    * */
    ~Thread();

private:

    /* the execution priority of the thread */
    int _priority;


    /* The state of the thread */
    STATES _state;

    /* the function the threads executes*/
    void (*_f)();

    /* the thread's ID  */
    int _tid;

    /* The total quantum performed by the thread */
    int _quantumCount;

    /* the memory of the thread */
    char *_stack;

    /* The environment the thread */
    sigjmp_buf env;
};


#endif //EX2_THREAD_H
