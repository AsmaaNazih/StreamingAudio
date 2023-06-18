#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <dirent.h>
#include "audio.h"
#include <time.h>
#include <sys/select.h>

int task(char data[]);
void sendMusic(char titre_audio[], int sock, struct sockaddr_in from);
void sendMessageError(int message, int sock, struct sockaddr_in from);

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

int main(int argc, char const *argv[])
{

	char *listeAudio[10];
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	int len, flen, err;
	char msgClient[32];
	char msgServer[32];
	struct sockaddr_in from;
	struct sockaddr_in addr;
	pid_t pid;
	Trame trame;

	//varibale pour lire le fichier audio
	int desA;
	flen = sizeof(from);


	if (sock<0)
	{
		printf("Error sock\n");
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	err = bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
	if (err < 0)
	{
		printf("Error bind\n");
	}
	

	while(1){
		memset(trame.type, '\0', sizeof(trame.type));
		len = recvfrom(sock, &trame, sizeof(trame), 0, (struct sockaddr*) &from, &flen);
		if (len<0) 
			printf("Error input connection\n");
		switch(task(trame.type)){
			case 0: printf("%s\n",trame.type);
					memset(trame.type, '\0', sizeof(trame.type));
					strcpy(trame.type, "Connected");
					sendto(sock, &trame, sizeof(trame), 0, (struct sockaddr*)&from, sizeof(struct sockaddr_in));
					break;
			case 1:
				char titre[16];
				memset(titre,'\0',sizeof(titre));
				strcat(titre, trame.titre);
				strcat(titre, ".wav"); 
				memset(trame.type, '\0', sizeof(trame.type));
				memset(trame.titre, '\0', sizeof(trame.titre));
				//nouveau socket pour transferer la musique
				int sockData = socket(AF_INET, SOCK_DGRAM, 0);
				pid = fork();
				if(pid==0)
					sendMusic(titre, sockData, from);
				break;
			case 2: printf("Error trame.type\n"); break;
			default: printf("error choix \n"); exit(1);break;
		}
	}

	return 0;
}


int task(char data[]){
	if(strcmp("Connect server on", data)==0)
		return 0;
	if(strcmp("titre", data)==0)
		return 1;
	return 2;
}

void sendMusic(char titre_audio[],int sock, struct sockaddr_in from){ 
	Trame trame;
	En_tete_audio eta;
	int desA;
	int flen, nb, max_fd, fclose;
	fd_set read_set;
	struct timeval timeout; 
	int hasDisconnected = 0;

	desA = aud_readinit(titre_audio, &eta.sample_rate, &eta.sample_size, &eta.channels);

	if(desA != -1){
		strcpy(trame.type, "en_tete_fichier_audio");
		sendto(sock, &trame, sizeof(trame), 0, (struct sockaddr*)&from, sizeof(struct sockaddr_in));
		sendto(sock, &eta, sizeof(eta), 0, (struct sockaddr*)&from, sizeof(struct sockaddr_in));
		memset(trame.titre,'\0',sizeof(trame.titre));
		strcpy(trame.type, "trame");
		trame.seq = 1;
		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
		max_fd = sock;
		while(read(desA, trame.buffer, eta.sample_rate)>0 && hasDisconnected < 5){
			sendto(sock, &trame, sizeof(trame), 0, (struct sockaddr*)&from, sizeof(struct sockaddr_in));
			timeout.tv_sec = 0;
			timeout.tv_usec = 5000000;
			nb = select(sock+1, &read_set, NULL, NULL, &timeout);
			if (nb == -1) {
            	perror("select");
            	exit(EXIT_FAILURE);
        	} else if (nb == 0 && hasDisconnected < 5) {
            	printf("Timeout !\n");
            	FD_ZERO(&read_set);
				FD_SET(sock, &read_set);
				hasDisconnected++;
            	continue;
        	}
			if(FD_ISSET(sock,&read_set))
				recvfrom(sock, &trame.seq, sizeof(int), 0, (struct sockaddr*) &from, &flen);
			trame.seq+=1;
		}   
		strcpy(trame.type, "fin_audio");
		sendto(sock, &trame, sizeof(trame), 0, (struct sockaddr*)&from, sizeof(struct sockaddr_in));
		//close socket
		fclose = close(sock);
		printf("disconnected\n");
		if (fclose < 0)	{
			printf("Error fermeture socket\n");
		}
	}else
		sendMessageError(0,sock,from);
}

void sendMessageError(int message, int sock, struct sockaddr_in from){
	Trame error;
	strcpy(error.type, "error");
	memset(error.buffer, '\0', sizeof(error.buffer));
	switch(message){
		case 0: strcpy(error.buffer, "audio file name not found");
			break;
		default: fprintf(stderr, "Error number entered\n");
	}
	sendto(sock, &error, sizeof(error), 0, (struct sockaddr*)&from, sizeof(struct sockaddr_in));
}