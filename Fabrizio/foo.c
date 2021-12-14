#include<stdio.h>
#include<stdlib.h>
#include<bool.h>


typedef struct packetsend{
	int32_t seqnumb;
	int32_t acknumb;
	bool overwritable;
	int position;
}packetsend;



int reorder(packetsend array[], int size){
	
	
  packetsend temp;
  // loop to access each array element
  for (int step = 0; step < size - 1; ++step) {
      
    // loop to compare array elements
		for (int i = 0; i < size - step - 1; ++i) {
		  
		  // compare two adjacent elements
		  // change > to < to sort in descending order
		  if (array[i].seqnumb > array[i + 1].seqnumb) {
			
			// swapping occurs if elements
			// are not in the intended order
			memccpy((void *)temp,(void *)array[i],sizeof(packetsend));
			memccpy((void *) array[i],(void *)array[i + 1],sizeof(packetsend));
			memccpy((void *array[i + 1],(void *)temp,sizeof(packetsend));

			}
		}
    }
	return 0;
}

// print array
void printArray(packetsend array[], int size) {
  for (int i = 0; i < size; ++i) {
    printf("in posizione %d\n seqnumb = %d\n acknumb = %d\n overwritable = %b\n position = %d\n", i,  packet[i].seqnumb, packet[i].acknumb, packet[i].overwritable,packet[i].position);
	printf("\n\n");
  }
  printf("\n");
}

int main(){
	
  packetsend packet[5];
  
  packet[0].seqnumb = 1234;
  packet[0].acknumb = 76543;
  packet[0].overwritable = 0;
  packet[0].position = 5;
  
  packet[1].seqnumb = 76;
  packet[1].acknumb = 87;
  packet[1].overwritable = 0;
  packet[1].position = 1;
  
  packet[2].seqnumb = 7654;
  packet[2].acknumb = 9083;
  packet[2].overwritable = 1;
  packet[2].position = 67;
  
  packet[3].seqnumb = 1235;
  packet[3].acknumb = 87;
  packet[3].overwritable = 0;
  packet[3].position = 4;
  
  packet[4].seqnumb = 1;
  packet[4].acknumb = 1;
  packet[4].overwritable = 1;
  packet[4].position = 10
  
  printf("Sorted Array :\n");
  printArray(packet, 5); 
  


  bubbleSort(packet, 5);
  
  printf("Sorted Array in Ascending Order:\n");
  printArray(packet, 5);
  
  return 0;
	
}







