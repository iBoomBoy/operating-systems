/**
 * This file implements parallel mergesort.
 */

#include <stdio.h>
#include <string.h> /* for memcpy */
#include <stdlib.h> /* for malloc */
#include "mergesort.h"

/* this function will be called by mergesort() and also by parallel_mergesort(). */
/* merge two sorted parts A[leftstart..leftend] and A[rightstart..rightend] put result in B first, then copy back to A */
void merge(int leftstart, int leftend, int rightstart, int rightend){
	int i = leftstart;
    int j = rightstart;
    int k = leftstart;

	/* i = left part, j = right part, k = where we write in B */
    /* compare front of both parts, take the smaller one */
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

	/* one part finished copy rest of the other part */
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

	/* copy sorted result from B back into A */
    memcpy(A + leftstart, B + leftstart,
           (rightend - leftstart + 1) * sizeof(int));
}

/* this function will be called by parallel_mergesort() as its base case. */
/* normal merge sort with one thread keep splitting till 1 number left, then merge back up*/
void my_mergesort(int left, int right){
	int mid;

	/* 0 or 1 number already sorted */
    if (left >= right) {
        return;
    }

	/* find middle, done this way so no int overflow */
    mid = left + (right - left) / 2;

	/* sort left half, sort right half, then merge them */
    my_mergesort(left, mid);
    my_mergesort(mid + 1, right);
    merge(left, mid, mid + 1, right);
}


/* this function will be called by the testing program. */
/*
 * each thread run this to sort its part A[left..right] if we hit cutoff -> just use my_mergesort, no more threads
 * if not -> cut in half, make 2 threads, wait for them, then merge
 * no lock needed, each thread have its own part of array, no overlap
 * parent free the child args after join
 * dont free arg here, test program already free the first one
 */
void * parallel_mergesort(void *arg){
    /* unpack the argument */
    struct argument *a = (struct argument *) arg;
    struct argument *leftarg;
    struct argument *rightarg;
    pthread_t leftthread;
    pthread_t rightthread;
    int leftcreated;
    int rightcreated;
    int left, right, level, mid;

    if (a == NULL) {
        return NULL;
    }

    left = a->left;
    right = a->right;
    level = a->level;

    /* 0 or 1 elements: already sorted */
    if (left >= right) {
        return NULL;
    }

    /* base case - cutoff reached 
	* sort this piece with one thread */
    if (level >= cutoff) {
        my_mergesort(left, right);
        return NULL;
    }

    /* split in half and build an argument for each child */
    mid = left + (right - left) / 2;
    leftarg = buildArgs(left, mid, level + 1);
    rightarg = buildArgs(mid + 1, right, level + 1);

    /* Job 4: create BOTH threads first so they run at the same time.
     * If pthread_create fails, we just call parallel_mergesort() directly for that half.
	 * This thread sorts that half itself, so the result is still correct. */
    leftcreated = (pthread_create(&leftthread, NULL, parallel_mergesort, leftarg) == 0);
    rightcreated = (pthread_create(&rightthread, NULL, parallel_mergesort, rightarg) == 0);

    if (!leftcreated) {
        parallel_mergesort(leftarg);
    }
    if (!rightcreated) {
        parallel_mergesort(rightarg);
    }

    /* then wait for both children to finish */
    if (leftcreated) {
        pthread_join(leftthread, NULL);
    }
    if (rightcreated) {
        pthread_join(rightthread, NULL);
    }

    /* children are done - free their arguments and merge the halves */
    free(leftarg);
    free(rightarg);
    merge(left, mid, mid + 1, right);

    return NULL;
}

/* we build the argument for the parallel_mergesort function. */
struct argument * buildArgs(int left, int right, int level){
	 struct argument *arg = malloc(sizeof(struct argument));

	/* malloc failed, print error and stop program */
    if (arg == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    arg->left = left;
    arg->right = right;
    arg->level = level;

    return arg;
}