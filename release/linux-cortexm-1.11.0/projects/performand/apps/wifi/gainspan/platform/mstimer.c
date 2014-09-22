/*-------------------------------------------------------------------------*
 * File:  mstimer.c
 *-------------------------------------------------------------------------*
 * Description:
 *     Using a timer, provide a one millisecond accurate timer.
 *-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
 * Includes:
 *-------------------------------------------------------------------------*/
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h> 
#include <time.h>
#include <stdio.h> 

#include "../hardware/GS_HAL.h"

/*-------------------------------------------------------------------------*
 * Globals:
 *-------------------------------------------------------------------------*/
/* 32-bit counter of current number of milliseconds since timer started */
volatile uint32_t G_msTimer;
struct timeval t1,t2;

/*---------------------------------------------------------------------------*
 * Routine:  MSTimerInit
 *---------------------------------------------------------------------------*
 * Description:
 *      Initialize and start the one millisecond timer counter.
 * Inputs:
 *      void
 * Outputs:
 *      void
 *---------------------------------------------------------------------------*/
void MSTimerInit(void)
{
   gettimeofday(&t1, NULL);
}

/*---------------------------------------------------------------------------*
 * Routine:  MSTimerGet
 *---------------------------------------------------------------------------*
 * Description:
 *      Get the number of millisecond counters since started.  This value
 *      rolls over ever 4,294,967,296 ms or 49.7102696 days.
 * Inputs:
 *      void
 * Outputs:
 *      uint32_t -- Millisecond counter since timer started or last rollover.
 *---------------------------------------------------------------------------*/
uint32_t MSTimerGet(void)
{
    uint32_t elapsedTime=0;
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    return elapsedTime;
}

/*---------------------------------------------------------------------------*
 * Routine:  MSTimerDelta
 *---------------------------------------------------------------------------*
 * Description:
 *      Calculate the current number of milliseconds expired since a given
 *      start timer value.
 * Inputs:
 *      uint32_t start -- Timer value at start of delta.
 * Outputs:
 *      uint32_t -- Millisecond counter since given timer value.
 *---------------------------------------------------------------------------*/
uint32_t MSTimerDelta(uint32_t start)
{
    return MSTimerGet() - start;
}

/*---------------------------------------------------------------------------*
 * Routine:  MSTimerDelay
 *---------------------------------------------------------------------------*
 * Description:
 *      Routine to idle and delay a given number of milliseconds doing
 *      nothing.
 * Inputs:
 *      uint32_t ms -- Number of milliseconds to delay
 * Outputs:
 *      void
 *---------------------------------------------------------------------------*/
void MSTimerDelay(uint32_t ms)
{
    //uint32_t start = MSTimerGet();

    //while (MSTimerDelta(start) < ms) {
    //}
    usleep(ms*1000);
}

/*-------------------------------------------------------------------------*
 * End of File:  mstimer.c
 *-------------------------------------------------------------------------*/

