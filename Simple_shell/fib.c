/*
 * No changes are allowed in this file
 */

#include<stdio.h>

int fib(int n) {
    if (n < 2) return n;
    else return fib(n - 1) + fib(n - 2);
}

int main() {
    int num;
    
    // Prompt the user for input
    printf("Enter a number: ");
    scanf("%d", &num);
    
    // Call the fib function and print the result
    int result = fib(num);
    printf("fib(%d) = %d\n", num, result);
    
    return 0;
}
