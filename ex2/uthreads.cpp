#include <map>
#include <iostream>
#include <sys/time.h>
#include <signal.h>
#include "uthreads.h"
#include "queue"
#include "Thread.h"
#include <list>

//====================================================================================================================//
//==================================================== Global parameter ==============================================//
//====================================================================================================================//

// Functions execution return codes:

/** Represents failure in function execution  **/
const static int FAIL = -1;

/** Represents success in function execution  **/
const static int SUCCESS = 0;;

// Units:

/** Conversion unit from second to microsecond  **/
const static int SECOND_TO_MICRO = 1000000;

// error messages:
/** Represents error given when librarry was not inited **/
const std::string LIBRARY_NOT_INIT = "librarry was not inited";

/** Represents error given when the sigset add was not successful **/
const std::string SIG_ADDSET_ERR = "an error occurred adding a set";

/** Represents error given with invalid quantom array **/
const std::string INVALID_QUANTUM_USECS = "quantum_usecs must be positive";

/** Represents error given with the signal empty **/
const static std::string SIGEMPTY_ERR = "an error has occurred trying to empty set";

/** Represents error given when quantum_usecs provided was null **/
const static std::string EMPTY_QUANTUM_USECS_ERR = "quantum usecs cannot be a null";

/** Represents an error with the sigaction **/
const static std::string SIGACTION_ERR = "sigaction error.";

/** Represents an error with the value range of the priority **/
const static std::string INVALID_PRIORITY_ERR = "invalid priority was given";

/** Represents error given when invalid size was provided(non positive)  **/
const static std::string INVALID_SIZE_ERR = "must enter a non negative size";

/** Represents error given when the maximal thread count was achieved  **/
const static std::string EXCEEDED_THREADS_ERR = "Thread counter is maximal!!";

/** Represents error given invalid thread id was given  **/
const static std::string INVALID_TID_ERR = "invalid thread id was given";

/** Represents error given when attempting to perform an action on a non existing thread   **/
const static std::string THREAD_EXISTENCE_ERR = "thread does not exist";

/** Represents error given when attempting to perform illegal action on the main thread **/
const static std::string MAIN_THREAD_PERMISSION_ERR = "the main thread cannot be chosen";

/** Represents error given when tje setitmer call failed **/
const static std::string TIMER_SET_ERR = "setitimer error.";

/** Represents error given when  a threads environment was not saved successfully **/
const static std::string SAVE_ENV_ERR = "enviroment was not saved correctly";

// Main thread :
const static int MAIN_THREAD_ID = 0;
const static int MAIN_THREAD_PRIORITY = 0;

/** The exit code of a bad return **/
const static int ERR_EXIT_CODE = 1;

// Thread management properties:

/** The current running thread **/
static Thread *_running = nullptr;

/** All the threads with ready state **/
static std::queue<Thread *> _ready;

/** All the threads with blocked state **/
static std::queue<Thread *> _blocked;

/** all the threads **/
static std::map<int, Thread *> _allThreads;

/** array representing the microseconds a given priority should have. **/
static int *_quantum_usecs = nullptr;

/** represents the length of  _quantum_usecs **/
static int _quantum_usecs_size = 0;

/** total number of quantoms executed by the proccess */
static int _quantum_count = 0;

/** The signal handler of the process **/
struct sigaction _sa = {0};

/** Timer to manage the runtime of threads**/
struct itimerval _timer;

//====================================================================================================================//
//==================================================== Global function definitions====================================//
//====================================================================================================================//

/***
 * Pops a specific element in a Queue or null_ptr
 * @param st a given Queue
 * @param tid the id of thread in the given Queue
 * @return the thread with the ginve tid
 */
Thread *popElementFromQueue(std::queue<Thread *> &st, int tid);

/**
 * Returns the next thread in the ready queue
 * **/
Thread *nextReadyThread();

/***
 * Blocks the running thread
 */
void pauseRunningThread();

/**
 * Returns the smallest non-negative integer
 * not already taken by an existing thread
 *  or -1 case no such thread can be found
 *
 * @return smallest non-negative integer not already taken by an existing thread
 * */
int getNextTID();

/**
 *  prints a single line when a function in the threads library fails
 * @param err the message to print
 */
void threadLibraryErrorMessage(const std::string &err);

/**
 * Prints a single line when a system call fails
 * @param err The type of error
 *
 * */
void systemCallErrorMessage(const std::string &err);

/**
 * Sets the timer to the current running thread
 *
 * **/
static void setTimer();

/****
 * Schedule handler
 * @param sig
 */
void timer_handler(int sig);

/****
 * Starts the running thread
 */
void startThread();

/****
 * Initialize a thread
 * @param priority
 * @param states
 * @param f
 * @return the id of the created thread
 */
int createThread(int priority, STATES states, void (*f)(void) = nullptr);

//====================================================================================================================//
//==================================================== Global functions ==============================================//
//====================================================================================================================//

/**
 * Retrieves a specific element from a queue
 * @param st 
 * @param tid 
 * @return 
 */
Thread *popElementFromQueue(std::queue<Thread *> &st, int tid)
{
    std::queue<Thread *> temp;
    //
    Thread *requiredThread = nullptr;

    while (st.size() != 0)
    {
        if (tid != st.front()->getTid())
        {
            temp.push(st.front());
        }
        else
        {
            requiredThread = st.front();
        }

        st.pop();

    }

    st.empty();

    while (temp.size() != 0)
    {

        st.push(temp.front());
        temp.pop();
    }

    return requiredThread;
}

/****
 * Gets the next thread in the ready queue
 * @return 
 */
Thread *nextReadyThread()
{

    Thread *thread_ptr;

    if (_ready.empty())
    {
        return nullptr;
    }
    else
    {
        thread_ptr = _ready.front();
        _ready.pop();
        return thread_ptr;
    }

}

/****
 * Moves the current running thread to the ready queue
 */
void pauseRunningThread()
{

    _running->setState(READY);
    if (!_running->saveEnvironment())
    {
        //todo
        exit(1);
    };
    _ready.push(_running);
    _running = nullptr;
}


/****
 * Gets the next appropriate id to create thread with
 * @return 
 */
int getNextTID()
{
    for (int i = 0; i < MAX_THREAD_NUM; i++)
    {
        if (_allThreads[i] == nullptr)
        {
            return i;
        }
    }

    return FAIL;

}

/**
 * Displays an error message of user error
 * @param err 
 */
void threadLibraryErrorMessage(const std::string &err)
{
    std::cerr << "thread library error: " << err << std::endl;
}

/**
 * Displays an error message of system call error
 * @param err 
 */
void systemCallErrorMessage(const std::string &err)
{
    std::cerr << "system error: " << err << std::endl;
    exit(ERR_EXIT_CODE);

}


/**
 * Sets the timer of the timer handler
 * 
 */
static void setTimer()
{

    // Gets the appropriate quamtom time
    int priority = _running->getPriority();
    int quantomTime = _quantum_usecs[priority];

    //  set the timer values
    _timer.it_interval.tv_sec = (int) (quantomTime / SECOND_TO_MICRO);
    _timer.it_interval.tv_usec = (int) (quantomTime % SECOND_TO_MICRO);
    _timer.it_value.tv_sec = (int) (quantomTime / SECOND_TO_MICRO);
    _timer.it_value.tv_usec = (int) (quantomTime % SECOND_TO_MICRO);


    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &_timer, NULL))
    {
        systemCallErrorMessage(TIMER_SET_ERR);
    }
}

void timer_handler(int sig)
{

    Thread *nextReady = nextReadyThread();
    if (nextReady == nullptr)
    {
        _running->increaseQuantum();
        _quantum_count++;
    }
    else
    {
        pauseRunningThread();
        _running = nextReady;
        startThread();

    }


}

/***
 * Executes the thread currently set as running 
 */
void startThread()
{

    _running->increaseQuantum();
    _running->setState(RUNNING);
    _quantum_count++;
    setTimer();
    _running->loadEnvironment();

}


/****
 * Creates a new thread
 * @param priority 
 * @param states 
 * @param f 
 * @return 
 */
int createThread(int priority, STATES states, void (*f)(void))
{
    int nextTID = getNextTID();
    Thread *thread = new Thread(priority, states, f, nextTID);
    _allThreads[nextTID] = thread;
    return nextTID;
}


/****
 * Checks whenever a quantum array contains only non negative values
 * @param quantum_usecs
 * @param size
 * @return
 */
int quantumIsNonNegative(int *quantum_usecs, const int &size)
{
    for (int i = 0; i < size; i++)
    {
        if (quantum_usecs[i] <= 0)
        {

            return FAIL;
        }
    }

    return SUCCESS;
}

//====================================================================================================================//
//==================================================== Library functions =============================================//
//====================================================================================================================//

int uthread_init(int *quantum_usecs, int size)
{

    // the size must be a non negative integer
    if (size <= 0)
    {
        threadLibraryErrorMessage(INVALID_SIZE_ERR);
        return FAIL;
    }

    if (quantumIsNonNegative(quantum_usecs, size) == FAIL)
    {
        threadLibraryErrorMessage(INVALID_QUANTUM_USECS);
        return FAIL;

    }

    if (quantum_usecs == nullptr)
    {
        threadLibraryErrorMessage(EMPTY_QUANTUM_USECS_ERR);
        return FAIL;
    }

    // setup the Quantum usecs
    _quantum_usecs = quantum_usecs;
    _quantum_usecs_size = size;

    _sa.sa_handler = timer_handler;

    if (sigemptyset(&_sa.sa_mask) == FAIL)
    {
        systemCallErrorMessage(SIGEMPTY_ERR);
    }

    if (sigaddset(&_sa.sa_mask, SIGVTALRM))
    {
        systemCallErrorMessage(SIG_ADDSET_ERR);
    }
    _sa.sa_flags = 0;


    if (sigaction(SIGVTALRM, &_sa, NULL) < 0)
    {
        systemCallErrorMessage(SIGACTION_ERR);
    }

    // create main thread
    int tid = createThread(MAIN_THREAD_PRIORITY, RUNNING, NULL);
    _running = _allThreads[tid];
    _running->increaseQuantum();
    _quantum_count++;
    setTimer();

    return SUCCESS;
}

int uthread_spawn(void (*f)(void), int priority)
{
    // create the thread and add to the ready queue
    sigprocmask(SIG_BLOCK, &_sa.sa_mask, NULL);
    if (_allThreads.size() == 0)
    {
        threadLibraryErrorMessage(LIBRARY_NOT_INIT);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

        return FAIL;
    }

    if (_allThreads.size() == MAX_THREAD_NUM)
    {
        threadLibraryErrorMessage(EXCEEDED_THREADS_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

        return FAIL;
    }

    if (_quantum_usecs_size < priority || MAIN_THREAD_PRIORITY > _quantum_usecs_size)
    {
        threadLibraryErrorMessage(INVALID_PRIORITY_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

        return FAIL;
    }


    int tid = createThread(priority, READY, f);
    _ready.push(_allThreads[tid]);
    sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

    return tid;
}

int uthread_change_priority(int tid, int priority)
{
    sigprocmask(SIG_BLOCK, &_sa.sa_mask, NULL);

    if (_quantum_usecs_size < priority || MAIN_THREAD_PRIORITY > _quantum_usecs_size)
    {
        threadLibraryErrorMessage(INVALID_PRIORITY_ERR);

    }
    else if (tid < 0 || tid >= MAX_THREAD_NUM)
    {
        threadLibraryErrorMessage(INVALID_TID_ERR);

    }
    else if (_allThreads[tid] == nullptr)
    {
        threadLibraryErrorMessage(INVALID_TID_ERR);
    }
    else
    {
        // updates the priority
        _allThreads[tid]->setPriority(priority);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

        return SUCCESS;

    }

    sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

    return FAIL;

}

int uthread_terminate(int tid)
{

    sigprocmask(SIG_BLOCK, &_sa.sa_mask, NULL);

    if (tid > MAX_THREAD_NUM && tid < 0)
    {
        threadLibraryErrorMessage(INVALID_TID_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

        return FAIL;
    }

    if (_allThreads[tid] == nullptr)
    {
        threadLibraryErrorMessage(THREAD_EXISTENCE_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return FAIL;
    }

    if (tid == MAIN_THREAD_ID)
    {
        _allThreads.clear();
        exit(0);
    }

    Thread *thread = _allThreads[tid];

    if (tid == _running->getTid())
    {

        // delete the current running thread
        delete _running;
        _allThreads.erase(tid);
        _running = nullptr;

        // insert a new thread
        Thread *thread = _ready.front();
        thread->setState(RUNNING);
        _running = thread;
        _ready.pop();
        startThread();

    }
    else
    {

        // remove all references
        popElementFromQueue(_ready, tid);
        popElementFromQueue(_blocked, tid);
        _allThreads.erase(tid);

        delete thread;

        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return SUCCESS;

    }

    sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

    return SUCCESS;
}

int uthread_block(int tid)
{
    sigprocmask(SIG_BLOCK, &_sa.sa_mask, NULL);

    if (tid == MAIN_THREAD_ID)
    {
        threadLibraryErrorMessage(MAIN_THREAD_PERMISSION_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return FAIL;
    }

    if (tid > MAX_THREAD_NUM && tid < 0)
    {
        threadLibraryErrorMessage(INVALID_TID_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return FAIL;
    }


    if (_allThreads[tid] == nullptr)
    {
        threadLibraryErrorMessage(THREAD_EXISTENCE_ERR);
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return FAIL;
    }

    if (_allThreads[tid]->getState() == BLOCKED)
    {
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return SUCCESS;
    }

    if (tid == _running->getTid())
    {
        // 1. save the content to  the running thread
        if (!_running->saveEnvironment())
        {
            sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
            systemCallErrorMessage(SAVE_ENV_ERR);
        };

        // 2. push the running to blocked
        Thread *oldRunning = _running;
        oldRunning->setState(BLOCKED);
        _blocked.push(oldRunning);
        _running = nullptr;

        // 3. go to the ready queue and set the next thread
        Thread *currentRunning = nextReadyThread();
        currentRunning->setState(RUNNING);
        _running = currentRunning;

        // 4. start the new thread
        startThread();

    }
    else
    {

        // 1. remove the thread from the ready queue
        Thread *thread = popElementFromQueue(_ready, tid);

        // 2. set the state of the thread to blocked
        thread->setState(BLOCKED);
        _blocked.push(thread);

    }

    sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
    return SUCCESS;
}

int uthread_resume(int tid)
{

    sigprocmask(SIG_BLOCK, &_sa.sa_mask, NULL);

    if (tid > MAX_THREAD_NUM && tid < 0)
    {
        threadLibraryErrorMessage(INVALID_TID_ERR);
    }
    else if (_allThreads[tid] == nullptr)
    {
        threadLibraryErrorMessage(THREAD_EXISTENCE_ERR);
    }
    else
    {

        Thread *thread = popElementFromQueue(_blocked, tid);

        // put into the Ready queue when blocked
        if (thread != nullptr)
        {
            thread->setState(READY);
            _ready.push(thread);
        }

        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

        return SUCCESS;

    }

    return FAIL;

}

int uthread_get_tid()
{
    return _running->getTid();
}

int uthread_get_total_quantums()
{

    return _quantum_count;
}

int uthread_get_quantums(int tid)
{
    sigprocmask(SIG_BLOCK, &_sa.sa_mask, NULL);

    if (tid > MAX_THREAD_NUM || tid < 0)
    {
        threadLibraryErrorMessage(INVALID_TID_ERR);
    }
    else if (_allThreads[tid] == nullptr)
    {
        threadLibraryErrorMessage(THREAD_EXISTENCE_ERR);;
    }
    else
    {
        int quantom = 0;
        quantom = _allThreads[tid]->getQuantumCount();
        sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);
        return quantom;
    }

    sigprocmask(SIG_UNBLOCK, &_sa.sa_mask, NULL);

    return FAIL;
}
