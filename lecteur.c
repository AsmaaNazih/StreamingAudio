 #include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include "audio.h"
#include <string.h>
#include <time.h>

#define MAX 30

int filtreDescriptor(int sample_rate, int sample_size, int channels, char filter[], char buffer[]);;
void volumeChange(char *buffer, float volume, int size);
void echo(char *bufferMain, char *bufferEcho, int size, int ind, int nbr);
void copy_to_2d_array(char source[], char dest[][48000], int index, int size); 

int main(int argc, char const *argv[]){

	int sample_rate;
	int sample_size;
	int channels;

	char *p = (char *)argv[1];
	char titre[MAX]="";
	
	int desA = -1;
	int desR = -1;
	int r,w;
	int len;

	/*si aucun titre précisé en ligne de commande, on demande à l'utilisateur 
	  de choisir une piste à jouée et on boucle tant que le fichier rentré est incorrect ou introuvé */
	while(desA==-1){
		if(p==NULL){
			printf("titre audio incorrect ou non choisi, \nRentrez un fichier audio: ");
			fgets(titre, MAX, stdin);
			len = strlen(titre);
			titre[len-1] = '\0';
		}else
			strcpy(titre,p);


		desA = aud_readinit(titre, &sample_rate, &sample_size, &channels);
		p=NULL;
	}

	// on affiche le titre de la piste audio
	printf("titre_audio :%s \n", titre);
	char filtre[32];
	float volume;

	memset(filtre, '\0', sizeof(filtre));

	if(argc>2){
		strcpy(filtre, (char *)argv[2]);
	}

	if(strcmp(filtre, "volume")==0){
		volume = atof(argv[3]);
		desR = aud_writeinit(sample_rate, sample_size, channels);
	}else{
		desR = filtreDescriptor(sample_rate, sample_size, channels, filtre, NULL);
		if(desR < 0 )
			exit(1);
	}

	int size = 48000;
	char buffer[size];
	char bufferEcho[25][size];
	pid_t pid;
	printf("desR : %d\n", desR);
	clock_t begin = clock();
	clock_t end = 0;
	int nbr = 0;
	int nbr2=0;
	int ind = 0;

	while(read(desA, buffer, size) > 0){
		if(strcmp(filtre, "volume")==0)
			volumeChange(buffer, volume, size);
		if(strcmp(filtre, "echo")==0){
			if((end-begin) > 7000){
				printf("test\n");
				begin = clock();
				ind++;
				nbr2=0;
			}
			
			//strcpy(bufferEcho[nbr++], buffer);
			copy_to_2d_array(buffer, bufferEcho, nbr, size);
			nbr++;
			
			if(nbr==25)
				nbr=0;
			if(ind==25)
				ind=0;
			echo(buffer, bufferEcho[ind], size, ind, nbr2++);
		}
		printf(" value : %d \n", nbr);
		end = clock();
		w = write(desR, buffer, size);
	}
	
	return 0;
}

int filtreDescriptor(int sample_rate, int sample_size, int channels, char filter[], char buffer[]){
	if(strcmp(filter, "rapide")==0)
		return aud_writeinit(sample_rate*2, sample_size, channels);
	if(strcmp(filter, "lent")==0)
		return aud_writeinit(sample_rate/2, sample_size, channels);	
	else
		return aud_writeinit(sample_rate, sample_size, channels);
}

void volumeChange(char *buffer, float volume, int size){
	for(int i = 0; i < size; i++){
		//printf("tab+ %d \n", buffer[i]);
		buffer[i]= buffer[i] * volume;
	}
}

void echo(char *bufferMain, char *bufferEcho, int size, int ind, int nbr){

    switch(nbr){
		case 0 : volumeChange(bufferEcho, 0.90, size);break;
		case 1 : volumeChange(bufferEcho, 0.80, size);break;
		case 2 : volumeChange(bufferEcho, 0.70, size);break;
		case 3 : volumeChange(bufferEcho, 0.60, size);break;
		case 4 : volumeChange(bufferEcho, 0.50, size);break;
	}

	for(int i = 0; i < size; i++){
		bufferMain[i]= bufferMain[i] + bufferEcho[i];
	}
}


void copy_to_2d_array(char source[], char dest[][48000], int index, int size) {
    int i;
    for (i = 0; i < size; i++) {
        dest[index][i] = source[i];
    }
    dest[index][i] = '\0';
}
