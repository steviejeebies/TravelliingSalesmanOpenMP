// mytour.h
// empty routine where students write their code

#ifndef MYTOUR_H
#define MYTOUR_H

// Just re-adding these here, but probably aren't needed. Won't affect performance,
// since they check if they have already been defined
#include <malloc.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "sales.h"

void my_tour(const point cities[], int tour[], int numCities);
float square(float x);
float distance(const point cities[], int i, int j);
#endif // MYTOUR_H
