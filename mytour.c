#include "mytour.h"

typedef struct {
    float x;
    float y;
    int cityNum;
} pointUpgrade;

void my_tour(const point cities[], int tour[], int numCities) {

    // Stephen Rowe ID 14319662
    // Oisin Tong
    
    // Here are some results we got from Stoker.
    // NumCity  Time
    // 100      429 microseconds
    // 200      1,111 microseconds
    // 400	    4,658 microseconds
    // 600	    6,927 microseconds
    // 800	    12,978 microseconds
    // 1000	    18,638 microseconds

    // 2000	    64,916 microseconds
    // 4000	    156,269 microseconds to  247,674 microseconds (these varied, could have been someone else using the machine at the same time)
    // 6000	    256,602 microseconds to 546,109 microseconds
    // 8000	    453,888 microseconds to 885,907 microseconds
    // 10,000	799,198 microseconds

    // 20,000	3,084,231 microseconds
    // 40,000	7.844.667 microseconds
    // 60,000	12,052,361 microseconds
    // 80,000	15,875,835 microseconds
    // 100,000	21,137,093 microseconds  (tested twice)
    // 500,000  150,432,012 microseconds (only tested this once)

    // The sweet spot on Stoker for enabling OpenMP was 26500. If
    // we enabled it for any lower than that, I got a time penalty.
    // !!! NOTE TO OISIN: When you're experimenting with scheduling for
    // the for-loops in this program, make sure to test if this value
    // needs to be updated. Turn OpenMP off entirely, time it, turn OpenMP
    // on constantly, time it, and rinse and repeat until you find the
    // sweet spot. Also, compare against the current version of this file.

    int OPEN_MP_ENABLED = (numCities > 26500) ? 1 : 0;
    int ClosePt;            // used to keep track of which city we're currently at, and a connected (close) city
    float CloseDist;        // distance between this city and the next
    int endTour = 0;                    // as we find close cities, we're adding it to the Tour array
    tour[endTour++] = numCities - 1;    // first Index of tour is the first city we've visited

    int citiesToCheck = numCities - 2;

//     Since we are not allowed to modify the sales.c code, I had to make the following
//     compromise. The 'point' struct from sales.h has the following structure:
//        typedef struct {
//            float x;
//            float y;
//        } point;
//    And the list of cities is a const, meaning we cannot modify the values within.
//
//    But I want to implement a version of the algorithm that does not rely on a
//    visited[] array. Instead, I use the variable citiesToCheck to keep track of how
//    many unvisited cities we need to check. When we have discovered the min-
//    distance city in our for-loop, I swap the largest-index unvisited city with the
//    discovered city. This means that each time we carry out the inner j-for-loop,
//    it will have one less iteration than the last time, down to just 1 city at the
//    very end. This gives a massive speed-boost, but there is one issue - if two
//    cities have the exact same distance from the current city, the original
//    algorithm chose the city with the lowest index. I need to replicate that, so I
//    had to make a new struct, 'pointUpgrade', which keeps track of the index of the
//    city, and chooses the lowest index one. This requires some extra if-statements
//    deeper into the program (explained below).
//
//    The Lab specification states "We will avoid inputs where the distances between
//    cities is the same (or almost the same) which might result in different answers
//    due to order of evaluation or floating point imprecision." From testing, I was
//    definitely getting cases where two cities could have the same distance from the
//    current city, so this means that if the test harness that this will be examined
//    against is guaranteed not to have these cases, then these added if-statements
//    (which will add to the running time) will be unnecessary. I'll make another
//    version of this program that omits the if-statements, please test that version
//    too, you should get a better time.
//    That file is called 'mytour_no_dist_check.c'
//
//    If modifying the struct above in sales.c was allowed, and if the list wasn't
//    constant, then I could skip instantiating this, and would get a better time.


    pointUpgrade * newCities = malloc(numCities * sizeof(pointUpgrade));

    #pragma omp parallel if(OPEN_MP_ENABLED)
    {
        #pragma omp for
        for (int i = 0; i < numCities; i++) {
            newCities[i].x = cities[i].x;
            newCities[i].y = cities[i].y;
            newCities[i].cityNum = i;
        }
    }

    // Instead of repeatedly getting the x and y coordinates of the current
    // city, we just get these once for each iteration of the i-for-loop,
    // then update them to the next city at the end of the iteration.

    float x = newCities[citiesToCheck+1].x;
    float y = newCities[citiesToCheck+1].y;

    // We cannot parallelize the outer i-for-loop, as each iteration has a dependency
    // (the dependency being the city that we are measuring distances from for the next
    // iteration).

    for (int i = 1; i < numCities; i++) {
        CloseDist = DBL_MAX;
        pointUpgrade * chosenCity;

        // We can parallelize the inner j-for-loop, as each iteration has no dependencies
        // on any other iteration.

        #pragma omp parallel if(OPEN_MP_ENABLED)
        {
            // Each thread needs a localCloseDist and localClosePt, so that we can
            // compare all of the threads' local minimums at the end to find the
            // absolute minimum
            float localCloseDist = DBL_MAX;
            int localClosePt;
            pointUpgrade * minCity;     // This thread's min-distance city
            pointUpgrade * thisCity;    // each iteration of the following j-for-loop points to another city
            float thisDist;

            // I've tried different combinations for scheduling here, e.g.
            // schedule(dynamic, X) or schedule(guided, Y), with different
            // values for X and Y, but nothing performed better than the
            // below default (i.e. schedule(static, 1)).
            #pragma omp for
            for (int j = 0; j <= citiesToCheck; j++) {
                // store memory access to local variable, to save on memory accesses
                // in the iteration of this loop.
                thisCity = &newCities[j];

                // Don't calculate sqrt(), it's unnecessary, as, if x < y, then sqrt(x) < sqrt(y).
                thisDist = square(x - thisCity->x) + square(y - thisCity->y);

                // I broke this if-statment into two. This first one will evaluate to true
                // a lot at the very start of this iteration for the for-loop, but taper off
                // very quickly. This means that, if thisDist > localCloseDist, then it will
                // fail into the next for-loop very quickly, no need to check the second
                // boolean for every greater-than loop (which it would have to do, considering
                // it would be an ||).
                if (thisDist <= localCloseDist) {
                    // Need the following if-statement to protect against the edge-case that, if
                    // two cities have equal distance to the current city, then the lower index
                    // city is chosen (to replicate the behaviour of the original Travelling Salesman
                    // code. thisDist < localCloseDist is very, very likely to be true, so
                    // we check this first, so we can skip the second boolean.
                    if(thisDist != localCloseDist || thisCity->cityNum < localClosePt) {
                        localClosePt = thisCity->cityNum;
                        minCity = thisCity;
                        localCloseDist = thisDist;
                    }
                }
            }

            #pragma omp critical
            {
                // Critical section, each thread has its own localCloseDist and minCity.
                // They will update the over-arching chosenCity to determine which city
                // we refer to for the next loop.

                if (localCloseDist <= CloseDist) {
                    // The following if-statement is very similar to the if-statement in
                    // the j-for-loop, we need to get the min-city with the lowest index,
                    // if distances are the same between current city and either of our two
                    // potential minimum cities. Again, we test the most-likely boolean
                    // first, so we can skip the second in the majority of cases.
                    if (localCloseDist != CloseDist || localClosePt < ClosePt) {
                        ClosePt = localClosePt;
                        CloseDist = localCloseDist;
                        chosenCity = minCity;
                    }
                }
            }
        }

        // The following needs to be done outside parallel loop, as each iteration of the
        // i-for-loop has to know which city we are starting from next. All this stuff is
        // critical too, and needs to be carried out by just the master thread.

        tour[endTour++] = chosenCity->cityNum;  // add the shortest city to the end of our current list of tours

        // We set the x and y coordinates of the current city for the next
        // iteration of the i-for-loop
        x = chosenCity->x;
        y = chosenCity->y;

        // We are going to swap the chosen min-distance city with the last index
        // of our unvisited cities. This involves simply setting the x, y, and cityNum
        // values of the chosen city to the values of the last-unvisited city. The
        // original values of the min-distance chosen city do not need to be maintained
        // beyond this point, as already-visited cities play no role in further iterations
        // in our algorithm.
        *chosenCity = *(&newCities[citiesToCheck--]);
        //pointUpgrade * shiftCity = &newCities[citiesToCheck--];
        //chosenCity->x = shiftCity->x;
        //chosenCity->y = shiftCity->y;
        //chosenCity->cityNum = shiftCity->cityNum;
    }
}

float square(float x) {
    return x * x;
}
