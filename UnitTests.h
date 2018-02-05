#include <mpi.h>
#include <string>
#include <functional>

void runTest(std::string testName, std::function<void(int,int)> test, int rank, int size);

void helloWorldTest(int rank, int size);

void pingPongTest(int rank, int size);

void nonBlockingTest(int rank, int size);
