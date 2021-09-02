//Burak Erdilli 19011046

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int refreshArray(int*, int, int);
bool headerInfo(FILE*, int*, int*);
void changePixelColor(int*, int);
void changeSpecificPixel(int*, int);
void printHistogram(int*, int);
void encoder(FILE*, int, int);
void decoder(int*, int);

bool headerInfo(FILE* fp, int* width, int* height){
	int bit_depth;	
	char image[5];
	fscanf(fp, "%s %d %d %d",image,width,height,&bit_depth);
	if(strcmp(image,"P2") == 0){
		printf("\n Header information of the file: \n\n");
		printf("%s %d %d %d\n",image,*width,*height,bit_depth);
		return true;
	}
	fclose(fp);
	return false;
}
int refreshArray(int* arrayPointer, int width, int height){
	FILE* fp = fopen("test_encoded.txt","r");
	
	if(fp){
		int lenght = 0;
		printf("\n\n");
		while(!feof(fp)){
			fscanf(fp, "%d ",&arrayPointer[lenght]);
			lenght++;
		}
		fclose(fp);
		return lenght;
	}
}
void encoder(FILE* fp, int width, int height){

	int index=1;
	int element1, element2;
	FILE* file = fopen("test_encoded.txt","w+");
	if(file){
		fprintf(file,"%d %d ", width, height);

		fscanf(fp, "%d", &element1);
		element2 = element1;
		if(!feof(fp)){
			while(!feof(fp)){
				fscanf(fp, "%d", &element1);
				if(element1 == element2){
					index++;
				}else{

					fprintf(file, "%d %d ", index, element2);
					index=1;
				}
				if(feof(fp)){
					index--;

					fprintf(file, "%d %d ", index, element1);
					fclose(file);
				}
				element2 = element1;
			}
		}else{

			fprintf(file, "%d %d ", index, element1);
			fclose(file);
		}
	}else{
		printf("\nError! File could not be opened.");
	}
}
void decoder(int* arrayPointer, int arraySize){
	int index = 0, pixelCount = 0, edgeLimit = 0;
	int sum = 0;
	int width, size;

	bool test = true;
	FILE* file; 
	size = arrayPointer[index]*arrayPointer[index+1];
	index = 2;
	for(index=2;index<arraySize;index+=2)
		sum+=arrayPointer[index];

	if(sum == size){
		printf("\n\nNumber of pixels in the original file and the pixel sum in the encoded file are matched, first control is successful!\n");
		index = 3;
		while( test==true&& index < arraySize ){
			if( arrayPointer[index] <= 255 && arrayPointer[index] >= 0){
				index += 2;
			}else{
				test = false;
				printf("\nError! Pixel's color value out of range, second control is failed!\n");	
			}
		}
		index = 2;
		if(test == true){
			printf("\nPixel's color range is valid, second control is successful!\n");
			while(index+2 < arraySize && test==true){
				if(arrayPointer[index+1] != arrayPointer[index+3]){
					index+= 2;
				}else{
					test = false;
					printf("\nError! File was not encoded correcty, third control is failed!\n");
				}
			}
			if(test == true){
				printf("\nFile is encoded correctly, third control is successful!\n ");
				index = 2;
				file = fopen("test_decoded.pgm","w+");
				width = arrayPointer[index-2];
				fprintf(file, "P2\n%d %d\n255\n", arrayPointer[index-2],arrayPointer[index-1]);
				printf("\nAll file controls succesfully passed! Starting to Decode...\n\n");
				for(index =2;index<arraySize;index+=2){
					while(pixelCount < arrayPointer[index]){
						if(edgeLimit <= width){
							edgeLimit++;
							fprintf(file,"%d ",arrayPointer[index+1]);
							pixelCount++;
							
						}else{
							fprintf(file,"\n");
							edgeLimit=0;
						}						
					}
					
					pixelCount = 0;
				}
				fclose(file);
				printf("\nFinished Decoding! test_decoded.pgm File is Available now!\n\n");
			}
		}
	}else{
		printf("\nError! Max pixel count doesn't match, first control is failed!\n");
	}
	
}
/*	
void changeSpecificPixel(int* arrayPointer, int arraySize){
	int index = 2;
	int pos_x, pos_y, value;
	int pos, sum = 0;
	int check = 0, tmp = 0;
	bool test=true;
	printf("\nPlease enter the coordinates: (x,y)");
	scanf("%d %d", &pos_x, &pos_y);
	printf("Please enter the pixel value: ");
	scanf("%d", &value);
	pos = arrayPointer[0]*pos_x + pos_y + 1;
	FILE* fp = fopen("test_encoded.txt","w+");
	
	if(fp){
		while(index < arraySize){
			sum += arrayPointer[index];
			if(sum >= pos && check != -1){
				check = pos + arrayPointer[index] - sum;
				while(check > 0){
					if(check == 1){
						if(tmp == 0){
							if(arrayPointer[index] == 0){
								fprintf(fp,"1 %d ",value);
								printf("1 %d ", value);
							}else{
								fprintf(fp,"1 %d %d %d ",value, arrayPointer[index]-1, arrayPointer[index+1]);
								printf("1 %d %d %d ", value, arrayPointer[index]-1, arrayPointer[index+1]);
							}
						}else{
							fprintf(fp,"%d %d ",tmp, arrayPointer[index]);
							printf("%d %d ", tmp, arrayPointer[index]);
							fprintf(fp,"1 %d ", value);
							printf("1 %d ", value);
							if(arrayPointer[index-1]-1 > 0){
							 	fprintf(fp,"%d %d ", arrayPointer[index-1]-1,arrayPointer[index]);
								printf("%d %d ", arrayPointer[index-1]-1,arrayPointer[index]);
							}
						}
						check = -1;
						arrayPointer[index+1]++;
						index = index + 2;
					}						
					arrayPointer[index-1]--;			
					tmp++;					
					check--;					
				}
			}else{
				fprintf(fp,"%d %d ",arrayPointer[index-1],arrayPointer[index]);
				printf("%d %d ",arrayPointer[index-1],arrayPointer[index]);
				index +=2;	
			}
		}
	}
}*/
void changePixelColor(int* arrayPointer, int arraySize){
    int oldPixel,newPixel;
    int index;
    bool test = false;
    printf("Please select color to change : \n");
    scanf("%d",&oldPixel);
	printf("Please type the new color: \n");
    scanf("%d",&newPixel);
    if(newPixel >= 0 && newPixel <= 255){
	    for(index=2;index<arraySize;index+=2){
	        if(arrayPointer[index+1]==oldPixel){
	        	test =true;
	            arrayPointer[index+1]=newPixel;	
	        }
	       
	    }
	    if(test==false){
	    	printf("\nError! The color you are looking for is not exist in the file \n");
		}
		else{
			FILE* fp = fopen("test_encoded.txt","w+");
			if(fp){	
				for(index = 0; index<arraySize; index++){
					fprintf(fp, "%d ",arrayPointer[index]);
					printf("%d ",arrayPointer[index]);
				}
				printf("\nSelected color is changed!\n");
				fclose(fp);
			}
			
		}
	}
}
void printHistogram(int* arrayPointer, int arraySize){
	float histogram[256] = {0};
	int index;
	float ratio,total;
	total=arrayPointer[0]*arrayPointer[1];
	for(index=2;index<arraySize;index+=2)
		histogram[arrayPointer[index+1]] += arrayPointer[index];
	
	index = 0;
	FILE* fp = fopen("test_encoded.txt","a");
	fprintf(fp,"\n");
	for(index=0;index<256;index++){
		if(histogram[index] != 0){
		
		ratio=(float) (histogram[index]/total);
		printf("\nTimes Used: %.0f , The Color: %d ,Color Ratio: %f percent\n", histogram[index],index,ratio*100.0);
			// fprintf(fp,"\nTimes Used:%d , The Color: %d\n", histogram[index],index);  histogramý dosyaya yazma islemi yapýlmýstýr ancak kaldýrýldý	
			}
	}
	fclose(fp);
}

int main(){
											
	
	bool exit=false;
    int width, height;
    char fileName[15];
	int arraySize,selection;
  	int* arrayPointer;
	FILE* fp;
	printf("\n------WELCOME------\n");
	while(exit==false){	
		printf("\nPlease select an option: (ex: 1,2,3) \n");
		printf("\n\n1--->File Encoding(Includes file operations on an encoded file)\n\n2--->File Decoding\n\n3--->Exit\n\n");
		printf("\nSelection:");
		scanf("%d", &selection);
		switch(selection){
		  	case 1:
		    	printf("\n Please type the file name(.pgm): ");
		    	scanf("%s",fileName);
		    	fp= fopen(fileName, "r");
		    	if(headerInfo(fp, &width, &height)){
		    		printf("\n Encoding the file...\n");
			        encoder(fp, width, height);
			        fclose(fp);
			        arrayPointer = (int*)malloc(width*height*sizeof(int));
			        arraySize = refreshArray(arrayPointer, width, height);
			        printf("\n Process is completed. \n\n");
			    	do{
			    		printf("\n\nPlease select an option for the test_encoded.txt file or return to main menu: \n");
						printf("\n1)Change Pixel Colors\n2)Change Pixel\n3)Print Histogram\n0)Main Menu\n");
						scanf("%d", &selection);
						switch(selection){
							case 1:
								changePixelColor(arrayPointer, arraySize);
								
								break;
							case 2:
								changeSpecificPixel(arrayPointer, arraySize);
								printf("\nThe pixel on the selected coordinates is changed.\n");
								break;
							case 3:
								printf("\nPrinting Histogram...\n");
								printHistogram(arrayPointer, arraySize);
								
								break;
							case 0:
								printf("\nReturning Main Menu...\n");
								break;
							default:
								printf("\n Please Try Again\n");
						}										
					}while(selection != 0);
				}else{
			        printf("\nError! File doesn't match.");
			    }
			    
		    	break;
		 	case 2:
				printf("\nPlease type the file name (.txt): ");
		    	scanf("%s",fileName);
		    	fp = fopen(fileName, "r");
		    	fscanf(fp,"%d %d",&width,&height);
				arrayPointer = (int*)malloc(width*height*sizeof(int));
				arraySize = refreshArray(arrayPointer, width, height);
			    decoder(arrayPointer,arraySize);
		    	break;
		    case 3:
		    	printf("\n Program Closing...");
		    	exit=true;
		    	break;
		    	return 0;
		 	default:
	    		printf("\nPlease Type Again: ");
	    }
	}
	return 0;
}
