#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/select.h>
#include <string.h>
#include "audio.h"
#include <time.h>

void sendMsg(int num, char *msg);
int task(char data[]);
int filtreDescriptor(int sample_rate, int sample_size, int channels, char filter[]);;
void volumeChange(char *buffer, float volume, int size);
void echo(char *bufferMain, char *bufferEcho, int size, int nbr);
void copy_to_2d_array(char source[], char dest[], int size);
void proposerFiltre();
int filtersChoice(char arg[]);

typedef struct En_tete_audio En_tete_audio;
struct En_tete_audio {
	int sample_rate;
	int sample_size;
	int channels;
};

typedef struct Trame Trame;
struct Trame {
	char type[32];
	char titre[16];
	int seq;
	char buffer[48000];
};

enum filtre {
	NORMAL=0, 
	VOLUME=1, 
	RAPIDE=2, 
	LENT=3, 
	ECHO=4
};

int main(int argc, char const *argv[]){

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	int fclose, err, len, flen;
	char msgClient[64];
	char ip[16];
	struct sockaddr_in serv;
	pid_t pid;
	key_t key = 1234;
	key_t keyV = 1235;
	key_t keyR = 1236;
	key_t keyL = 1237;
	key_t keyN = 1238;
	key_t keyB = 1239;
	int *desN, *desR, *desL, *des;
	Trame trame;
	int shmid, shmidF, shmidV, shmidR, shmidL, shmidN, shmidB;
	int *inProgress,*filtre;
	int arret = 0, nb, firstConnection=0;
	fd_set read_set;
	struct timeval timeout;
	int connected = 0;
	float *value;
	En_tete_audio eta;

	if (sock < 0)
	{
		printf("Error socket\n");
	}

	serv.sin_family		 = AF_INET;
	serv.sin_port		 = htons(1234);
	serv.sin_addr.s_addr = inet_addr((char *)argv[1]); 

	//memoire partagée
	shmid = shmget(IPC_PRIVATE, sizeof(int), 0600|IPC_CREAT);
	shmidF = shmget(key, sizeof(int), 0600|IPC_CREAT);
	shmidV = shmget(keyV, sizeof(float), 0600|IPC_CREAT);
	shmidR = shmget(keyR, sizeof(int), 0600|IPC_CREAT);
	shmidL = shmget(keyL, sizeof(int), 0600|IPC_CREAT);
	shmidN = shmget(keyN, sizeof(int), 0600|IPC_CREAT);
	shmidB = shmget(keyB, sizeof(int), 0600|IPC_CREAT);

	if(shmid < 0 || shmidF < 0 || shmidV < 0 || shmidR < 0 || shmidL < 0 || shmidN < 0 || shmidB < 0){
		perror( "Error shmget \n"); exit(1);
	}
	
	inProgress = (int*) shmat(shmid,NULL,0);
	filtre = (int*) shmat(shmidF,NULL,0);
	value = (float*) shmat(shmidV,NULL,0);
	desR = (int*) shmat(shmidR,NULL,0);
	desL = (int*) shmat(shmidL,NULL,0);
	desN = (int*) shmat(shmidN,NULL,0);
	des = (int*) shmat(shmidB,NULL,0);

	*value = 1;

	//connect to serv 
	sendMsg(0, trame.type);
	FD_ZERO(&read_set);
	FD_SET(sock, &read_set);
	while (err < 0 || !connected){
		err = sendto(sock, &trame, sizeof(trame), 0, (struct sockaddr*) &serv, sizeof(serv));
		timeout.tv_sec = 0;
		timeout.tv_usec = 5000000;
		nb = select(sock+1, &read_set, NULL, NULL, &timeout);
		if (nb == -1) {
        	perror("select");
        	exit(EXIT_FAILURE);
    	} else if (nb == 0) {
        	printf("Timeout !\n");
        	FD_ZERO(&read_set);
			FD_SET(sock, &read_set);
			printf("Error IP address \n Enter valide Ip address : \n");
			fgets(ip, 16, stdin);
			ip[strlen(ip)-1] = '\0';
			serv.sin_addr.s_addr = inet_addr(ip); 
        	continue;
    	}
		if(FD_ISSET(sock,&read_set)){
			recvfrom(sock, &trame, sizeof(trame), 0, (struct sockaddr*) &serv, &flen);
			printf("Connected!\n");
			connected = 1;
		}

	}



	pid = fork();
	if(pid==0){
							//processus fils  ****GESTION INPUT****
		int size = 48000;
		char bufferEcho[size];
		time_t begin = time(NULL);
		time_t end;
		int vol = 0;
		while(arret == 0){
			len = recvfrom(sock, &trame, sizeof(trame), 0, (struct sockaddr*) &serv, &flen);

			if (len<0) { 
				printf("Error len\n");
			}
			trame.type[len] = '\0';

			switch(task(trame.type)){
				case 0:
					recvfrom(sock, &eta, sizeof(eta), 0, (struct sockaddr*) &serv, &flen);
					*desN = filtreDescriptor(eta.sample_rate, eta.sample_size, eta.channels, "");
					*desL = filtreDescriptor(eta.sample_rate, eta.sample_size, eta.channels, "lent");
					*desR = filtreDescriptor(eta.sample_rate, eta.sample_size, eta.channels, "rapide");
					switch(*filtre){
						case 3 : *des = *desL;break;
						case 2 : *des = *desR;break;
						default : *des = *desN;  
					}

					while(len = recvfrom(sock, &trame, sizeof(trame), 0,(struct sockaddr*) &serv, &flen)>0 && strcmp(trame.type, "trame")==0){
						end = time(NULL);
						if(*filtre==VOLUME){
							volumeChange(trame.buffer, *value, size);
						}
						if(*filtre==ECHO){
							if(difftime(end, begin) > 0.99){
								begin = time(NULL);
								vol=0;
								copy_to_2d_array(trame.buffer, bufferEcho, size);
							}
							echo(trame.buffer, bufferEcho, size, vol++);
						}
						end = clock();
						write(*des, trame.buffer, eta.sample_rate);
						sendto(sock, &trame.seq, sizeof(int), 0, (struct sockaddr*) &serv, sizeof(serv));
					}
					printf("%s\n", trame.type);
					printf("Choisissez votre musique ou tapez \"stop\" pour quitter: \n");
					*inProgress = 0;
					break;
				case 1: printf(" Message server : %s \n",trame.buffer); 
						*inProgress = 0;
						break;
				default: exit(1);break;
			}	
		}
	}else{
							//processus père ****GESTION OUTPUT****
		while(arret == 0){
			if(*inProgress==0){
				memset(trame.type, '\0', sizeof(trame.type));
				memset(trame.titre, '\0', sizeof(trame.type));
				strcpy(trame.type, "titre");
				if(argc>=3 && firstConnection==0){
					strcpy(trame.titre, (char *)argv[2]);
					if(argc>3)
						*filtre= filtersChoice((char *)argv[3]);
					if(*filtre==VOLUME && argc>4)
						*value = atof(argv[4]);

					firstConnection=1;
				}
				else{
					printf("Choisissez votre musique ou tapez \"stop\" pour quitter: \n");
					fgets(trame.titre, 16, stdin);
					trame.titre[strlen(trame.titre)-1] = '\0';
					if(strcmp(trame.titre, "stop")==0)
						arret=1;
				}
				if(arret==0){
					err = sendto(sock, &trame, sizeof(trame), 0, (struct sockaddr*) &serv, sizeof(serv));
					*inProgress=1;
				}
				if (err < 0){
					printf("Error sendto\n");
					exit(1);
				}
			}

			if(arret ==0 && *inProgress==1){
				proposerFiltre();
				scanf("%d", filtre);
				if(*filtre==VOLUME){
					printf("indiquez une valeur entre 0 et 2 (float)\n");
					scanf("%f", value);
				}
				if(*filtre==LENT)
					*des = *desL;
					
				if(*filtre==RAPIDE)
					*des = *desR;

				if(*filtre==NORMAL)
					*des = *desN;
			}
			
		}
	}

	//fermeture socket, suppression segment memoire partagée
	close(*desN);
	close(*desR);
	close(*desL);
	shmdt(inProgress);
	shmdt(filtre);
	shmdt(desR);
	shmdt(value);
	shmdt(des);
	shmdt(desL);
	shmdt(desN);
	shmctl(shmidL, IPC_RMID, NULL);
	shmctl(shmidB, IPC_RMID, NULL);
	shmctl(shmidN, IPC_RMID, NULL);
	shmctl(shmidV, IPC_RMID, NULL);
	shmctl(shmidR, IPC_RMID, NULL);
	shmctl(shmidF, IPC_RMID, NULL);
	shmctl(shmid, IPC_RMID, NULL);
	fclose = close(sock);
	if (fclose < 0)	{
		printf("Error fermeture socket\n");
	}
	printf("ByeBye!!!\n");
	return 0;
}

void sendMsg(int num, char msg[]){
	char connect[32] = "Connect server on";
	switch(num){
		case 0 : strcpy(msg, connect);break;
		case 1: break;
		default: "error";
	}
}

int task(char data[]){

	if(strcmp("en_tete_fichier_audio", data)==0)
		return 0;
	if(strcmp("error", data)==0)
		return 1;
	if (strcmp("connected", data)==0)
		return 2;
}

int filtreDescriptor(int sample_rate, int sample_size, int channels, char filter[]){
	if(strcmp(filter, "rapide")==0)
		return aud_writeinit(sample_rate*2, sample_size, channels);
	if(strcmp(filter, "lent")==0)
		return aud_writeinit(sample_rate/2, sample_size, channels);	
	else
		return aud_writeinit(sample_rate, sample_size, channels);
}

void volumeChange(char *buffer, float volume, int size){  
	for(int i = 0; i < size; i++){
		buffer[i]= buffer[i] * volume;
	}
}

void echo(char *bufferMain, char *bufferEcho, int size, int nbr){

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


void copy_to_2d_array(char source[], char dest[], int size) {
    int i;
    for (i = 0; i < size; i++) {
        dest[i] = source[i];
    }
}

void proposerFiltre(){
	printf("Choisissez un filtre :\n");
	printf("0. normal\n");
	printf("1. volume\n");
	printf("2. rapide\n");
	printf("3. lent\n");
	printf("4. echo\n");
}

int filtersChoice(char arg[]){
	if(strcmp(arg, "lent")==0)
		return LENT;
	if(strcmp(arg, "rapide")==0)
		return RAPIDE;
	if(strcmp(arg, "echo")==0)
		return ECHO;
	if(strcmp(arg, "volume")==0)
		return VOLUME;

	printf("nom de filtre incorrect \n");
	return NORMAL;
}
