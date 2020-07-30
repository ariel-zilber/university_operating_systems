#ifndef MAPREDUCEFRAMEWORK_H
#define MAPREDUCEFRAMEWORK_H

#include "MapReduceClient.h"

typedef void *JobHandle;

enum stage_t
{
    UNDEFINED_STAGE = 0, MAP_STAGE = 1, SHUFFLE_STAGE = 2, REDUCE_STAGE = 3
};

typedef struct
{
    stage_t stage;
    float percentage;
} JobState;


/****
 * This function produces a (K2*,V2*) pair.
 * @param key
 * @param value
 * @param context
 */
void emit2(K2 *key, V2 *value, void *context);

/***
 * This function produces a (K3*,V3*) pair.
 * @param key
 * @param value
 * @param context
 */
void emit3(K3 *key, V3 *value, void *context);


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
JobHandle startMapReduceJob(const MapReduceClient &client,
                            const InputVec &inputVec, OutputVec &outputVec,
                            int multiThreadLevel);

/****
 *
 * @param job a function gets the job handle returned by ​ startMapReduceFramework and
 *  waits until it is finished.
 */
void waitForJob(JobHandle job);

/***
 * this function gets a job handle and updates the state of the job into the given
 * JobState struct.
 * @param job
 * @param state
 */
void getJobState(JobHandle job, JobState *state);

/***
 * Releasing all resources of a job. You should prevent releasing resources
 *  before the job is finished
 * @param job
 */
void closeJobHandle(JobHandle job);


#endif //MAPREDUCEFRAMEWORK_H
