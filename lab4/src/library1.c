#include "libraries.h"


// Евклид
int gcd(int a, int b) {
    int temp;
    while (b != 0) {
        temp = b;
        b = a % b;
        a = temp;
    }
    return a; 
}

// Пузырьковая
int *sort(int *array, size_t n) {
    int tmp, noSwap;

    for (int i = n - 1; i >= 0; --i) {
        noSwap = 1;
        for (int j = 0; j < i; ++j) {
            if (array[j] > array[j + 1]) {
                tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
                noSwap = 0;
            }
        }
        if (noSwap) break;
    }
    
    return array;
}