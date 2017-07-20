#include "AveragingFilter.h"
#include <stdio.h>
#include <string.h>

AveragingFilter::AveragingFilter(){
    memset(buffer, 0, sizeof(uint16_t));
}

AveragingFilter::~AveragingFilter(){
    
}

void AveragingFilter::feedFilter(uint16_t newVal){
    for (int i = 0; i < NUM_ELEMENTS-1; i++){
        buffer[i] = buffer[i+1];
    }
    buffer[NUM_ELEMENTS-1] = newVal;
}

double AveragingFilter::getAverage(){
    uint32_t sum = 0;
    for (int i = 0; i < NUM_ELEMENTS; i++){
        sum += buffer[i];
    }
    return ((double)sum)/NUM_ELEMENTS;
}
