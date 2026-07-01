#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

extern int SIZE;

struct DataItem {
   char* key;   
   int ValueS;

   struct DataItem* h_next;
};

extern struct DataItem** hashArray; 

void initialize_Hash(int s);
long hashCode(char *str);
struct DataItem *search(char* key); 
void insertHashEntry(char* key,int size);