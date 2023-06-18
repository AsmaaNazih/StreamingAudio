#
# Link
#

lecteur	:	lecteur.o audio.o
	gcc -o lecteur lecteur.o audio.o
	
audioserver:	audioserver.o audio.o
	gcc -o audioserver audioserver.o audio.o
	
audioclient:	audioclient.o audio.o
	gcc -o audioclient audioclient.o audio.o
 		

#
# Objets of TP Lists
#


lecteur.o: lecteur.c audio.h
	gcc -c lecteur.c -o lecteur.o  
	
audioserver.o:	audioserver.c audio.h
	gcc -c audioserver.c -o audioserver.o
	
audioclient.o:	audioclient.c audio.h
	gcc -c audioclient.c -o audioclient.o

audio.o:	audio.c audio.h
	gcc -o audio.o -c audio.c 
	

all: lecteur audioserver audioclient	

#
# Remove files
#
clean :
		rm *.o
		rm lecteur
		rm audioserver
		rm audioclient	
	
