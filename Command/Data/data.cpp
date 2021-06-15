#include <iostream>
#include <cstdlib>
#include <time.h>

int main(){
    //generating a random number:
    srand((unsigned int)time(NULL));

    std::cout << rand() % 65536 << "," << rand() % 65536 << "," << rand() % 361 << "," <<  rand() % 65536 << "," << rand() % 65536 << std::endl;
    std::cout << rand() % 101 << "," << rand() % 101 << "," << rand() % 66536 << std::endl;
    std::cout << rand() % 2 << "," << rand() % 65536 << "," << rand() % 65536 << std::endl;
    std::cout << rand() % 2 << "," << rand() % 65536 << "," << rand() % 65536 << std::endl;
    std::cout << rand() % 2 << "," << rand() % 65536 << "," << rand() % 65536 << std::endl;
    std::cout << rand() % 2 << "," << rand() % 65536 << "," << rand() % 65536 << std::endl;
    std::cout << rand() % 2 << "," << rand() % 65536 << "," << rand() % 65536 << std::endl;
}
