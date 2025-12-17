#include "libraries.h"


// Наивная
int gcd(int a, int b) {
    int min = (a < b) ? a : b;
    int res = 1;
    for (int i = 1; i <= min; ++i) {
        if (a % i == 0 && b % i == 0) {
            res = i;
        }
    }

    return res;
}


// Хоар
void quick_sort(int *array, int left, int right) {
    if (left >= right) return;

    int pivot = array[(left + right) / 2];
    int i = left;
    int j = right;
    int tmp;

    while (i <= j) {
        while (array[i] < pivot) ++i;
        
        while (array[j] > pivot) --j;

        if (i <= j) {
            tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
            ++i;
            --j;
        }
    }

    if (left < j) quick_sort(array, left, j);
    if (i < right) quick_sort(array, i, right);
}

int *sort(int *array, size_t n) {
    if (n > 0) quick_sort(array, 0, (int)n - 1);
    return array;
}