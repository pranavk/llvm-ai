#include <iostream>

int main (int argc, char* argv[], char* envp[]) {
   int k = 42;
   int a = k + k;
   int d = (a + a) - (a - a);
   return (d + a);
}
