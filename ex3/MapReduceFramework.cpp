#include "MapReduceFramework.h"
#include <pthread.h>
#include <atomic>
#include <iostream>
#include <set>
#include "Barrier.cpp"

//--------------------------------------------------------- Constants ------------------------------------------------//

/** **/
static const std::string SYSTEM_ERR_MSG = "system error: ";
/** **/
static const std::string INVALID_THREAD_COUNT = "not enouth threads were given";

/** exit error code**/
static const int EXIT_ERR_CODE = 1;

/** error message on thread init **/
static const std::string THREAD_INIT_ERR = "An error occurred attempting  initializing the thread";

/** error message on mutex init **/
static const std::string MUTEX_INIT_ERR = "An error occurred attempting  initializing the mutex";

/** mutex related error message**/
const std::string MUTEX_ERROR = "MUTEX LOCK ERROR";

/** error on pthread join**/
const std::string PTHREAD_JOIN_ERR = "pthread join error";

/** exit system on error**/
void systemError(const std::string &errorMsg);

//--------------------------------------------------------- Inner structs/classes-------------------------------------//
typedef std::vector<IntermediatePair> IntermediateVec;

/***
 * Includes all the parameters which are relevent to this job
 */
struct JobContext;

/***
 * Represents context of a thread
 */
struct ThreadContext;

struct ThreadContext
{
    ThreadContext(int tid, JobContext *context)
            :
            _orderID(tid), _context(context), _intermediateVec(new IntermediateVec()), _terminated(false)
    {

    }


    std::atomic<int> _orderID;

    JobContext *_context;
    IntermediateVec *_intermediateVec;
    bool _terminated;

    ~ThreadContext()
    {
        delete _intermediateVec;
    }

};

struct JobContext
{
    /***
     *
     * @param client
     * @param inputVec
     * @param outputVec
     * @param multiThreadLevel number of threads the job has
     */
    JobContext(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec,
               const int &multiThreadLevel) :
            _threads(new pthread_t[multiThreadLevel]),
            _shuffleVecMutex(new pthread_mutex_t),
            _reduceVecMutex(new pthread_mutex_t),
            _barrier(new Barrier(multiThreadLevel)),
            _intermediateVec(new IntermediateVec()),
            _intermediateMap(new IntermediateMap()),
            _intermediateCount(0),
            _uniqueK2Vector(std::vector<K2 *>()),
            _uniqueK2Map(std::map<K2 *, bool, K2PointerComp>()),
            _uniqueK2(0),
            _threadCount(multiThreadLevel),
            _currMapTask(0),
            _currShuffleTask(0),
            _currReduceTask(0),
            _MapTaskDone(0),
            _ShuffleTaskDone(0),
            _ReduceTaskDone(0),
            _stage(UNDEFINED_STAGE),
            _client(client),
            _inputVec(inputVec),
            _outputVec(outputVec),
            _threadsContext(new std::vector<ThreadContext *>())
    {

        if ((pthread_mutex_init(_shuffleVecMutex, nullptr) != 0))
        {
            systemError(MUTEX_INIT_ERR);

        };
        if ((pthread_mutex_init(_reduceVecMutex, nullptr) != 0))
        {
            systemError(MUTEX_INIT_ERR);
        };

        for (int i = 0; i < multiThreadLevel; i++)
        {
            _threadsContext->push_back(new ThreadContext(i, this));
        }
    }


    /***
     * Destructor
     */
    ~JobContext()
    {

        if ((pthread_mutex_destroy(_shuffleVecMutex) != 0))
        {
            systemError(MUTEX_INIT_ERR);
        };

        if ((pthread_mutex_destroy(_reduceVecMutex) != 0))
        {
            systemError(MUTEX_INIT_ERR);
        };

        while (_threadsContext->size() != 0)
        {
            delete _threadsContext->back();
            _threadsContext->pop_back();
        }


        delete[]_threads;
        delete _shuffleVecMutex;
        delete _reduceVecMutex;
        delete _barrier;
        delete _intermediateVec;
        delete _intermediateMap;
        delete _threadsContext;
    }

    /** **/

    pthread_t *_threads;

    // Mutexes:

    /** shuffle/map phase mutex**/
    pthread_mutex_t *_shuffleVecMutex;
    pthread_mutex_t *_reduceVecMutex;

    Barrier *_barrier;
    IntermediateVec *_intermediateVec;
    IntermediateMap *_intermediateMap;
    std::atomic<int> _intermediateCount;
    std::vector<K2 *> _uniqueK2Vector;
    std::map<K2 *, bool, K2PointerComp> _uniqueK2Map;
    std::atomic<int> _uniqueK2;


    //counters:

    /** the total amount of threads **/
    int _threadCount;

    /** the current mapping **/
    std::atomic<int> _currMapTask;
    std::atomic<int> _currShuffleTask;
    std::atomic<int> _currReduceTask;

    /** how many map tasks were done **/
    std::atomic<int> _MapTaskDone;
    std::atomic<int> _ShuffleTaskDone;
    std::atomic<int> _ReduceTaskDone;


    /** the current job state **/
    std::atomic<stage_t> _stage;

    // consts:

    /** The calling client**/
    const MapReduceClient &_client;

    /** The calling client's input**/
    const InputVec &_inputVec;

    /** The output vector**/
    OutputVec &_outputVec;

    /** Represents the context of all the job's threads**/
    std::vector<ThreadContext *> *_threadsContext;
};

//--------------------------------------------------------- Inner functions  -----------------------------------------//

/****
 *  Displays a system error then exists
 * @param errorMsg
 */
void systemError(const std::string &errorMsg);

/***
 * Runs the Map action
 * @param threadContext
 */
void runMap(ThreadContext *threadContext);

/***
 * Runs the shuffle action
 * @param threadContext
 */
void runShuffle(ThreadContext *threadContext);

/***
 * Executes the reduce algorithm on a threadContext
 * @param threadContext  a given thread context.
 */
void runReduce(ThreadContext *threadContext);

/***
 * Runs the map reduce program
 * @param context  A pointer to the job context
 * @return
 */
void *runMapReduce(void *context);

void systemError(const std::string &errorMsg)
{
    std::cerr << SYSTEM_ERR_MSG << errorMsg << std::endl;
    exit(EXIT_ERR_CODE);
}

void runMap(ThreadContext *threadContext)
{
    unsigned int old_value;

    if (threadContext->_context->_stage.load() != MAP_STAGE)
    {
        threadContext->_context->_stage.store(MAP_STAGE);
    }

    while ((threadContext->_context->_MapTaskDone) < (int) threadContext->_context->_inputVec.size())
    {

        //   DEBUG("mapping");

        old_value = ((threadContext->_context->_MapTaskDone)).fetch_add(1);

        if (old_value < threadContext->_context->_inputVec.size())
        {
            threadContext->_context->_client.map(threadContext->_context->_inputVec[old_value].first,
                                                 threadContext->_context->_inputVec[old_value].second,
                                                 threadContext);

            (threadContext->_context->_currMapTask).fetch_add(1);
        }
    }
}

void runShuffle(ThreadContext *threadContext)
{
    // run shuffle
    JobContext *jobContext = threadContext->_context;
    const int numOfThreads = jobContext->_threadCount;
    bool isDone = false;
    IntermediateVec *vec;

    while (!isDone)
    {

        for (int i = 0; i < numOfThreads; i++)
        {

            while (!(*jobContext->_threadsContext)[i]->_intermediateVec->empty())
            {
                if (pthread_mutex_lock(jobContext->_shuffleVecMutex) != 0)
                {
                    systemError(MUTEX_ERROR);
                }

                vec = (*jobContext->_threadsContext)[i]->_intermediateVec;

                // holds a map of unique k2
                if (!jobContext->_uniqueK2Map[vec->back().first])
                {
                    jobContext->_uniqueK2Map[vec->back().first] = true;
                    jobContext->_uniqueK2Vector.push_back(vec->back().first);
                    (*jobContext->_intermediateMap)[vec->back().first] = std::vector<V2 *>();
                    jobContext->_uniqueK2++;
                }


                (*jobContext->_intermediateMap)[vec->back().first].push_back(vec->back().second);
                jobContext->_currShuffleTask++;

                // removes the last element
                vec->pop_back();

                if (pthread_mutex_unlock(jobContext->_shuffleVecMutex) != 0)
                {
                    systemError(MUTEX_ERROR);
                }
            }

            // case all the mapping threads have finished
            if (jobContext->_barrier->getCount() == numOfThreads - 1)
            {
                jobContext->_stage = SHUFFLE_STAGE;
            }

            // check finish condition
            if ((jobContext->_currShuffleTask == jobContext->_intermediateCount)
                && (jobContext->_stage == SHUFFLE_STAGE))
            {
                isDone = true;
            }

        }


    };


}

void runReduce(ThreadContext *threadContext)
{
    // update the current stage to reduce
    threadContext->_context->_stage = REDUCE_STAGE;

    int currJob = (threadContext->_context->_currReduceTask)++;
    JobContext *context = threadContext->_context;

    std::vector<K2 *> *uniqueK2Vector = &context->_uniqueK2Vector;
    K2 *currK2;
    std::vector<V2 *> currVector;

    // iterate until no more jobs are left
    while (currJob < context->_uniqueK2)
    {
        currK2 = (*uniqueK2Vector)[currJob];
        currVector = (*context->_intermediateMap)[currK2];
        context->_client.reduce(currK2, currVector, threadContext);

        // update the task counter
        currJob = (threadContext->_context->_currReduceTask)++;
        (threadContext->_context->_ReduceTaskDone)++;
    }


}

void *runMapReduce(void *context)
{
    auto *threadContext = (ThreadContext *) context;
    const int shuffleThead = threadContext->_context->_threadCount - 1;

    // Map/Shuffle Phase:
    if (threadContext->_orderID.load() != shuffleThead)
    {
        runMap(threadContext);
    }
    else
    {
        runShuffle(threadContext);
    }

    // waits for all threads to finish
    threadContext->_context->_barrier->barrier();

    // Reduce Phase:
    runReduce(threadContext);


    return nullptr;

}

//---------------------------------------------------- Library function ----------------------------------------------//

/****
 * This function starts running the MapReduce algorithm
 * (with several threads) and returns a JobHandle.
 * @param client The implementation of ​ MapReduceClient class,
 * in other words the task that the framework should run.
 * @param inputVec  a vector of type std::vector<std::pair<K1*, V1*>>, the input elements
 * @param outputVec a vector of type std::vector<std::pair<K3*, V3*>>, to which the output
    elements will be added before returning.
    It may be assumed that the output vector can be empty
 * @param multiThreadLevel the number of worker threads to be used for running the algorithm.
 * @return
 */
JobHandle
startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec, int multiThreadLevel)
{
    JobContext *context;

    // should have a valid parameter count
    if (multiThreadLevel < 2)
    {
        systemError(INVALID_THREAD_COUNT);
    }

    // Initialize the context
    context = new JobContext(client, inputVec, outputVec, multiThreadLevel);

    // Start running the job's threads
    for (int i = 0; i < context->_threadCount; i++)
    {
        if (pthread_create(&context->_threads[i], nullptr, runMapReduce, (void *) context->_threadsContext->at(i)) != 0)
        {
            delete context;
            systemError(THREAD_INIT_ERR);
        };
    }

    return context;
}

/****
 *
 * @param job a function gets the job handle returned by ​ startMapReduceFramework and
 *  waits until it is finished.
 */
void waitForJob(JobHandle job)
{
    auto jobContext = (JobContext *) job;
    auto threads = jobContext->_threads;

    for (int i = 0; i < (int) jobContext->_threadCount; ++i)
    {
        if ((!(*jobContext->_threadsContext)[i]->_terminated))
        {
            if (pthread_join(threads[i], nullptr) != 0)
            {
                systemError(PTHREAD_JOIN_ERR);
            }
            (*jobContext->_threadsContext)[i]->_terminated = true;
        }
    }
}

/***
 * this function gets a job handle and updates the state of the job into the given
 * JobState struct.
 * @param job
 * @param state
 */
void getJobState(JobHandle job, JobState *state)
{
    auto *context = (JobContext *) job;
    stage_t stage = context->_stage.load();
    if (stage == UNDEFINED_STAGE)
    {
        state->stage = UNDEFINED_STAGE;
        state->percentage = 0;
    }
    else if (stage == MAP_STAGE)
    {
        auto totalK1 = (float) context->_inputVec.size();
        float currK1 = context->_MapTaskDone.load();
        state->stage = MAP_STAGE;
        state->percentage = (currK1 / totalK1) * 100;
    }
    else if (stage == SHUFFLE_STAGE)
    {
        auto totalK1 = (float) context->_intermediateCount.load();
        float currK1 = context->_currShuffleTask.load();
        state->stage = SHUFFLE_STAGE;
        state->percentage = (currK1 / totalK1) * 100;
    }
    else if (stage == REDUCE_STAGE)
    {
        float totalK2 = (float) context->_uniqueK2.load();
        float currK2 = context->_ReduceTaskDone.load();;
        state->stage = REDUCE_STAGE;
        state->percentage = (currK2 / totalK2) * 100;
    }
}

/***
 * Releasing all resources of a job. You should prevent releasing resources
 *  before the job is finished
 * @param job
 */
void closeJobHandle(JobHandle job)
{
    waitForJob(job);
    delete (JobContext *) job;
}

/****
 * This function produces a (K2*,V2*) pair.
 * @param key
 * @param value
 * @param context
 */
void emit2(K2 *key, V2 *value, void *context)
{

    auto *threadContext = (ThreadContext *) context;

    if (pthread_mutex_lock(threadContext->_context->_shuffleVecMutex) != 0)
    {
        systemError(MUTEX_ERROR);
    }

    threadContext->_intermediateVec->push_back(std::pair<K2 *, V2 *>(key, value));
    threadContext->_context->_intermediateCount++;

    if (pthread_mutex_unlock(threadContext->_context->_shuffleVecMutex) != 0)
    {
        systemError(MUTEX_ERROR);
    }

}

/***
 * This function produces a (K3*,V3*) pair.
 * @param key
 * @param value
 * @param context
 */
void emit3(K3 *key, V3 *value, void *context)
{
    auto *threadContext = (ThreadContext *) context;

    if (pthread_mutex_lock(threadContext->_context->_reduceVecMutex) != 0)
    {
        systemError(MUTEX_ERROR);
    }

    threadContext->_context->_outputVec.push_back({key, value});

    if (pthread_mutex_unlock(threadContext->_context->_reduceVecMutex) != 0)
    {
        systemError(MUTEX_ERROR);
    }
}
