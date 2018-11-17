//
// rndDriver.cpp random test driver
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//

#include "precomp.h"

typedef struct _FUNCTION_RECORD {
    RNDD_TEST_FN    func;
    char *          name;
    volatile INT64  count;
    UINT32          weight;
    } FUNCTION_RECORD, *PFUNCTION_RECORD;

#define N_BUCKETS   (1 << 12)

//
// We have a fixed # buckets that point to the function records.
// This allows a very quick selection of the next test to run, and the precision of the
// probabilities can be controlled by the # buckets that we use.
//
PFUNCTION_RECORD  g_aBucket[N_BUCKETS];

UINT32  g_totalWeight;

std::vector<FUNCTION_RECORD *> g_testFunctions;

std::vector<FUNCTION_RECORD *> g_initFunctions;

std::vector<FUNCTION_RECORD *> g_cleanupFunctions;

std::vector<FUNCTION_RECORD *> g_invariantFunctions;

BOOL g_invariantFunctionsUsed = FALSE;

C_ASSERT( (N_BUCKETS & (N_BUCKETS-1)) == 0 );       // must be a power of 2

VOID
recomputeBuckets()
{
    UINT32 b = 0;
    UINT32 w = 0;

    CHECK( g_totalWeight < (1 << 16 ), "Too much total weight, could lead to overflow" );

    for( std::vector<FUNCTION_RECORD *>::iterator i = g_testFunctions.begin(); i != g_testFunctions.end(); i++ )
    {
        w += (*i)->weight;
        while( w * N_BUCKETS > g_totalWeight * b )
        {
            CHECK( b < N_BUCKETS, "?" );
            g_aBucket[ b ] = *i;
            b += 1;
        }
    }
}


VOID
rnddRegisterTestFunction( RNDD_TEST_FN func, char * name, UINT32 weight )
{
    PFUNCTION_RECORD p = new FUNCTION_RECORD;
    p->func = func;
    p->name = name;
    p->weight = weight;
    p->count = 0;
    g_testFunctions.push_back( p );

    g_totalWeight += weight;
}

VOID
rnddRegisterInitFunction( RNDD_TEST_FN func )
{
    PFUNCTION_RECORD p = new FUNCTION_RECORD;

    p->func = func;
    p->weight = 0;
    g_initFunctions.push_back( p );
}   


VOID
rnddRegisterCleanupFunction( RNDD_TEST_FN func )
{
    PFUNCTION_RECORD p = new FUNCTION_RECORD;

    p->func = func;
    p->weight = 0;
    g_cleanupFunctions.push_back( p );
}   

VOID
rnddRegisterInvariantFunction( RNDD_TEST_FN func )
{
    PFUNCTION_RECORD p = new FUNCTION_RECORD;

    p->func = func;
    p->weight = 0;
    g_invariantFunctions.push_back( p );

    g_invariantFunctionsUsed = TRUE;
}   

ULONGLONG
getTimeInMs()    // Will have to move it to the main_exe or main_dll when we support kernel mode
{
    return GetTickCount64();
}

VOID
rnddRunTest( UINT32 nSeconds, UINT32 nThreads )
{
    ULONGLONG timeLimit;

    dprint( "\n" );
    recomputeBuckets();

    CHECK( 0 < nThreads && nThreads <= 1, "?" );    // Currently only 1 thread supported
    CHECK( 0 < nSeconds && nSeconds < 100, "Invalid test duration" );

    for( std::vector<FUNCTION_RECORD *>::iterator i = g_initFunctions.begin(); i != g_initFunctions.end(); i++ )
    {
        (*((*i)->func))();
    }

    timeLimit = getTimeInMs() + nSeconds * 1000;

    do {
        for( int i=0; i<1000; i++ )
        {
            UINT32 c = g_rng.uint32() & (N_BUCKETS - 1 );
            dprint( "%21s:", g_aBucket[c]->name );
            (*g_aBucket[c]->func)();
            InterlockedIncrement64( &g_aBucket[c]->count );

            if( g_invariantFunctionsUsed )
            {
                for( std::vector<FUNCTION_RECORD *>::iterator i = g_invariantFunctions.begin(); i != g_invariantFunctions.end(); i++ )
                {
                    (*((*i)->func))();
                }
            }
            dprint( "\n" );
        }
    } while( getTimeInMs() < timeLimit );

    print( "\n" );
    for( std::vector<FUNCTION_RECORD *>::iterator i = g_testFunctions.begin(); i != g_testFunctions.end(); i++ )
    {
        print( "%30s : %I64d\n", (*i)->name, (*i)->count );
    }
    iprint( "\n" );

    for( std::vector<FUNCTION_RECORD *>::iterator i = g_cleanupFunctions.begin(); i != g_cleanupFunctions.end(); i++ )
    {
        (*((*i)->func))();
    }

}

