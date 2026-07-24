#include <stdio.h> 
#include <string.h> 
#include "ting_runtime.h" 
int main(); 
int main(){ 
printf("%s", "Hello! welcome to my first program. pick an option between 1-4. \n");
printf("%s", "1: \n");
printf("%s", "2: \n");
printf("%s", "3: \n");
printf("%s", "4: Exit \n");
printf("%s", "Enter: ");
int choice = input_int(); 
while (((choice < 1) || (choice > 4))){ 
printf("%s", "Option ");
printf("%d", choice);
printf("%s", " does not exist... please choose again. \n");
printf("%s", "Enter: ");
choice = input_int(); 
} 
while ((choice != 4)){ 
if ((choice == 1)){ 
printf("%s", "tomer is meow \n");
} 
else if ((choice == 2)){ 
printf("%s", "meow 2 \n");
} 
else if ((choice == 3)){ 
printf("%s", "meow 3 \n");
} 
printf("%s", "Enter: ");
choice = input_int(); 
while (((choice < 1) || (choice > 4))){ 
printf("%s", "Option ");
printf("%d", choice);
printf("%s", " does not exist... please choose again. \n");
printf("%s", "Enter: ");
choice = input_int(); 
} 
} 
printf("%s", "Bye bye! \n");
return 0; 
} 
