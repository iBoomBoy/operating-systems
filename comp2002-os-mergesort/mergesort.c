/**
 * This file implements parallel mergesort.
 */

#include <stdio.h>
#include <string.h> /* for memcpy */
#include <stdlib.h> /* for malloc */
#include "mergesort.h"

/* this function will be called by mergesort() and also by parallel_mergesort(). */
void merge(int leftstart, int leftend, int rightstart, int rightend){
	int i = leftstart;
    int j = rightstart;
    int k = leftstart;

    while (i <= leftend && j <= rightend) {
        if (A[i] <= A[j]) {
            B[k] = A[i];
            i++;
        } else {
            B[k] = A[j];
            j++;
        }
        k++;
    }

    while (i <= leftend) {
        B[k] = A[i];
        i++;
        k++;
    }

    while (j <= rightend) {
        B[k] = A[j];
        j++;
        k++;
    }

    memcpy(A + leftstart, B + leftstart,
           (rightend - leftstart + 1) * sizeof(int));
}

/* this function will be called by parallel_mergesort() as its base case. */
void my_mergesort(int left, int right){
	int mid;

    if (left >= right) {
        return;
    }
    mid = left + (right - left) / 2;
    my_mergesort(left, mid);
    my_mergesort(mid + 1, right);
    merge(left, mid, mid + 1, right);
}

/* this function will be called by the testing program. */
void * parallel_mergesort(void *arg){
		return NULL;
}

/* we build the argument for the parallel_mergesort function. */
struct argument * buildArgs(int left, int right, int level){
	 struct argument *arg = malloc(sizeof(struct argument));

    if (arg == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    arg->left = left;
    arg->right = right;
    arg->level = level;

    return arg;
}

