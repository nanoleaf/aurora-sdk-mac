#include <stdint.h>

#define NUM_ELEMENTS 10

class AveragingFilter{
    uint16_t buffer[NUM_ELEMENTS];
    int numElements;
public:
    AveragingFilter();
    ~AveragingFilter();
    void feedFilter(uint16_t);
    double getAverage();
    
};
