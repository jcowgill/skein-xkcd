#include <stdio.h>
#include <stdint.h>
#include "skein.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

// Counts bits in a byte
static const unsigned char BitsSetTable256[256] = 
{
#   define B2(n) n,     n+1,     n+1,     n+2
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
};

// Reference result and best global result
uint8_t resultRef[0x80];
int bestResultGlobal = 1000000;

// Locks
#ifdef WIN32

static CRITICAL_SECTION lockVar;
#define enterLock EnterCriticalSection
#define leaveLock LeaveCriticalSection

#else

static pthread_mutex_t lockVar = PTHREAD_MUTEX_INITIALIZER;
#define enterLock pthread_mutex_lock
#define leaveLock pthread_mutex_unlock

#endif

void processingThread(uint8_t firstChar)
{
    // Setup state
    Skein1024_Ctxt_t ctx;
    uint8_t result[0x80];
    uint8_t data[0x10];

    int i;
    int bitsDifferent;
    int bestResult = 100000;

    // Wipe data
    for (i = 1; i < 0x10; i++)
        data[i] = 'B';
    data[0] = firstChar;

    // Process
    for (;;)
    {
        // Try hashing this
        Skein1024_Init(&ctx, 1024);
        Skein1024_Update(&ctx, data, 0x10);
        Skein1024_Final(&ctx, result);

        // Compare with reference
        bitsDifferent = 0;
        for (i = 0; i < 0x80; i++)
            bitsDifferent += BitsSetTable256[result[i] ^ resultRef[i]];

        // Better?
        if (bitsDifferent < bestResult)
        {
            enterLock(&lockVar);
            {
                // Check global result
                if (bitsDifferent < bestResultGlobal)
                {
                    // Update global result
                    bestResultGlobal = bitsDifferent;

                    // Print value
                    printf("%.3i: %.16s\n", bitsDifferent, data);
                }

                // Update our best result
                bestResult = bestResultGlobal;
            }
            leaveLock(&lockVar);
        }

        // Advance other bytes
        for (i = 0xF; i > 0; i--)
        {
            if (++data[i] != ']')
                break;
            else
                data[i] = 'A';
        }
    }
}

#ifdef WIN32

DWORD WINAPI processingThreadWinApi(LPVOID param)
{
    processingThread((uint8_t) param);
    return 0;
}

#else

void * processingThreadPthreads(void * param)
{
    processingThread((uint8_t) param);
    return NULL;
}

#endif

void setupResultRef(void)
{
    // Populate result reference
    int i;
    const char * str = "5b4da95f5fa08280fc9879df44f418c8f9f12ba424b7757de02bbdfbae0d4c4fdf9317c80cc5fe04c6429073466cf29706b8c25999ddd2f6540d4475cc977b87f4757be023f19b8f4035d7722886b78869826de916a79cf9c94cc79cd4347d24b567aa3e2390a573a373a48a5e676640c79cc70197e1c5e7f902fb53ca1858b6";

    for (i = 0; i < 0x80; i++)
    {
        // Get both bytes
        char a = str[2 * i];
        char b = str[2 * i + 1];

        // Adjust values
        if (a < 'a')
            a -= '0';
        else
            a -= ('a' - 10);

        if (b < 'a')
            b -= '0';
        else
            b -= ('a' - 10);

        // Store result
        resultRef[i] = (a << 4) + b;
    }
}

// Main routine
int main(void)
{
#ifdef WIN32

    DWORD i;
    SYSTEM_INFO info;

    Skein1024_Ctxt_t ctx;
    uint8_t result[0x80];

    Skein1024_Init(&ctx, 1024);
    Skein1024_Update(&ctx, "VBBBBBBBCINHT\\XW", 0x10);
    Skein1024_Final(&ctx, result);


    setupResultRef();

    // Setup lock and idleness
    InitializeCriticalSectionEx(&lockVar, 256, 0);
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

    // Get number of processors
    GetSystemInfo(&info);

    // Start threads
    for (i = 0; i < info.dwNumberOfProcessors; i++)
        CreateThread(NULL, 0, processingThreadWinApi, (LPVOID) (0x41 + i), 0, NULL);

    // Suspend
    Sleep(INFINITE);

#else

    int i, nrCpus;
    
    setupResultRef();

    // I'm very nice
    nice(10000);

    // Get number of processors (sorry BSD!)
    nrCpus = sysconf(_SC_NPROCESSORS_ONLN);

    // Start threads
    for (i = 0; i < nrCpus; i++)
        pthread_create(malloc(sizeof(pthread_t)), NULL, processingThreadPthreads, (void*) (0x41 + i));

    // Suspend
    sleep(UINT_MAX);

#endif

    return 0;
}