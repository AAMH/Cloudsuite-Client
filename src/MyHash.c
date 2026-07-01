#include "MyHash.h"

int SIZE;
struct DataItem** hashArray;

void initialize_Hash(int s){
    SIZE = s;
    hashArray = (struct DataItem**) malloc(SIZE * sizeof(struct DataItem*));
    memset(hashArray,0,SIZE * sizeof(struct DataItem*));
    printf("\nHashTable initialized. Size: %d\n",SIZE);
}

long hashCode(char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % SIZE;
}

struct DataItem *search(char* key) {

    int hashIndex = hashCode(key);  
    struct DataItem* curr = hashArray[hashIndex];

    while(curr != NULL && strcmp(curr->key,key)){
        curr = curr->h_next;
    }
    return curr; 
   //move in array until an empty 
//    while(hashArray[hashIndex] != NULL) {
	
//       if(hashArray[hashIndex]->key == key)
//          return hashArray[hashIndex]; 
			
//       //go to next cell
//       ++hashIndex;
		
//       //wrap around the table
//       hashIndex %= SIZE;
//    }        
	
//    return NULL;        
}

void insertHashEntry(char* key,int size) {
    
    struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
    memset(item,0,sizeof(struct DataItem));
    item->key = key;  
    item->ValueS = size;
    
    int hashIndex = hashCode(key);
    struct DataItem* curr = hashArray[hashIndex];
    struct DataItem* prev = NULL;

    if(curr == NULL)
        hashArray[hashIndex] = item;
    else{
        while(curr != NULL){
            prev = curr;
            curr = curr->h_next;
        }
        prev->h_next = item;
    }
    //printf("\nInserting: %s and %d, HashIndex = %d\n",key,size,hashIndex);
   //move in array until an empty or deleted cell
//    while(hashArray[hashIndex] != NULL) {
//       //go to next cell
//       ++hashIndex;
		
//       //wrap around the table
//       hashIndex %= SIZE;
//    }
//     if(hashArray[hashIndex] != 0)
//         printf("OVerwriting... Previous Size:%d  New Size:%d\n",hashArray[hashIndex]->ValueS,size);
//    hashArray[hashIndex] = item;
}