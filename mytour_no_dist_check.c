#include "mytour.h"

typedef struct {
    float x;
    float y;
    int cityNum;
} pointUpgrade;

// Same as other mytour.c, but this version assumes that if our current city is
// City A, and City B is distance x from City A, then there does not exist a City
// C that has that exact same distance, in any case. If this isn't guaranteed for the
// test cases, then this version will succeed for relatively-low number of cities,
// but will get more and more likely to fail to match the sales.c tour as the
// number of cities gets larger (>40000) (it'll still give a completely valid
// answer for the min-distance algorithm, it just won't be identical to sales.c)
// Although, one run for X cities could succeed in matching, and another run with 
// the same X number of cities could fail to match, with X being a very large number.

void my_tour(const point cities[], int tour[], int numCities) {
    int OPEN_MP_ENABLED = (numCities > 26500) ? 1 : 0;
    int ClosePt;
    float CloseDist;
    int endTour = 0;
    tour[endTour++] = numCities - 1;
    int citiesToCheck = numCities - 2;

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
    float x = newCities[citiesToCheck+1].x;
    float y = newCities[citiesToCheck+1].y;

    for (int i = 1; i < numCities; i++) {
        CloseDist = DBL_MAX;
        pointUpgrade * chosenCity;

        #pragma omp parallel if(OPEN_MP_ENABLED)
        {
            float localCloseDist = DBL_MAX;
            int localClosePt;
            pointUpgrade * minCity;
            pointUpgrade * thisCity;
            float thisDist;
            #pragma omp for
            for (int j = 0; j <= citiesToCheck; j++) {
                thisCity = &newCities[j];
                thisDist = square(x - thisCity->x) + square(y - thisCity->y);
                if (thisDist < localCloseDist) {
                    localClosePt = thisCity->cityNum;
                    minCity = thisCity;
                    localCloseDist = thisDist;
                }
            }

            #pragma omp critical
            {
                if (localCloseDist < CloseDist) {
                    ClosePt = localClosePt;
                    CloseDist = localCloseDist;
                    chosenCity = minCity;
                }
            }
        }
        tour[endTour++] = chosenCity->cityNum;
        x = chosenCity->x;
        y = chosenCity->y;
        *chosenCity = *(&newCities[citiesToCheck--]);
    }
}

float square(float x) {
    return x * x;
}
