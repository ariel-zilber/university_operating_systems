#include "osm.h"
#include <sys/time.h>

/**
 * The number of loop unrolling operations to be done
 * */
#define LOOP_UN 5

/**
 * Return value representing a function returning an error value
 */
#define FAILURE -1


/**
 * A default value for timezone
 * */
#define DEFAULT_TZ nullptr

/**
 * Unit to convert from second to milli second
 */
const double SECONDS_TO_MILLI = 1000;

/**
 * Unit to convert from second to micro second
 */
const double SECONDS_TO_MICRO = 1000000;

/**
 * Unit to convert from second to nano second
 */
const double SECONDS_TO_NANOS = 1000000000;


/**
 * An empty function that is supposed to be used as measurements
 */
void emptyFunction()
{}


/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations)
{

    // check the validity of the input
    if (iterations <= 0)
    {
        return FAILURE;
    }

    // used in the arithmetic function
    int temp = 0;

    timeval timeBeforeAction{};
    timeval timeAfterAction{};

    double totalDurationSec;

    //
    int remainder = iterations % LOOP_UN ? 1 : 0;

    //
    if (gettimeofday(&timeBeforeAction, DEFAULT_TZ))
    {
        return FAILURE;
    }

    // iterate over the arithmetic action
    for (unsigned int i = 0; i < iterations / LOOP_UN + remainder; i++)
    {
        temp = 1 + 1;
        temp = 1 + 1;
        temp = 1 + 1;
        temp = 1 + 1;
        temp = 1 + 1;
    }

    if (gettimeofday(&timeAfterAction, DEFAULT_TZ) == FAILURE)
    {
        return FAILURE;
    }

    // Calc the total number of ms that the code took:
    totalDurationSec = (timeAfterAction.tv_sec - timeBeforeAction.tv_sec) +
                       ((timeAfterAction.tv_usec - timeBeforeAction.tv_usec) / SECONDS_TO_MICRO);

    return totalDurationSec * SECONDS_TO_NANOS;
}


/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations)
{

    // check the validity of the input
    if (iterations <= 0)
    {
        return FAILURE;
    }

    timeval timeBeforeAction{};
    timeval timeAfterAction{};

    double totalDurationSec;

    int remainder = iterations % LOOP_UN ? 1 : 0;

    if (gettimeofday(&timeBeforeAction, DEFAULT_TZ))
    {
        return FAILURE;
    }

    // iterate over the arithmetic action
    for (unsigned int i = 0; i < iterations / LOOP_UN + remainder; i++)
    {
        emptyFunction();
        emptyFunction();
        emptyFunction();
        emptyFunction();
        emptyFunction();
    }

    if (gettimeofday(&timeAfterAction, DEFAULT_TZ) == FAILURE)
    {
        return FAILURE;
    }

    // Calc the total number of ms that the code took:
    totalDurationSec = (timeAfterAction.tv_sec - timeBeforeAction.tv_sec) +
                       ((timeAfterAction.tv_usec - timeBeforeAction.tv_usec) / SECONDS_TO_MICRO);

    return totalDurationSec * SECONDS_TO_NANOS;

}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations)
{

    // check the validity of the input
    if (iterations <= 0)
    {
        return FAILURE;
    }

    timeval timeBeforeAction{};
    timeval timeAfterAction{};

    double totalDurationSec;

    //
    int remainder = iterations % LOOP_UN ? 1 : 0;

    //
    if (gettimeofday(&timeBeforeAction, DEFAULT_TZ))
    {
        return FAILURE;
    }

    // iterate over the arithmetic action
    for (unsigned int i = 0; i < iterations / LOOP_UN + remainder; i++)
    {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
    }

    if (gettimeofday(&timeAfterAction, DEFAULT_TZ) == FAILURE)
    {
        return FAILURE;
    }

    // Calc the total number of ms that the code took:
    totalDurationSec = (timeAfterAction.tv_sec - timeBeforeAction.tv_sec) +
                       ((timeAfterAction.tv_usec - timeBeforeAction.tv_usec) / SECONDS_TO_MICRO);

    return totalDurationSec * SECONDS_TO_NANOS;
}

