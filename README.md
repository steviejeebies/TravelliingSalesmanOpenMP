# TravelliingSalesmanOpenMP
Version of the TravellingSalesman problem, implemented in OpenMP.

This version eliminates the check to see if a city has been visited, by creating a modifiable list of cities, in which unvisited cities are at the front of the list, and visited cities are at the end of the list (swap index of city we are about to visit with the last city that is unvisited, and reduce the count of unvisited cities). As a result, this drastically reduces the number of calculations that has to be carried out by the program. OpenMP will not be enabled for a low number of cities (<26500), as it gives no benefit compared to a simple version for number of cities less than this.
