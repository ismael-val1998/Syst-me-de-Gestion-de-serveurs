#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define NB_RESSOURCE 3 //ressources du serveur production


sem_t sInteg, sProd, sBackup, ressourceProd, sTestServer;
pthread_t integ, backup, prod; //threads représentant les 3 serveurs
int serveur;
int nbErreur = 0, nbCommEnvoyes = 0, nbCommProd = 0, nbCommBackup = 0; //variables globales qui nous sert pour les statistiques

const char *commandPtoB="diff -a --suppress-common-lines -y logProd.txt logBackup.txt > logBackup.txt";
const char *commandBtoP="diff -a --suppress-common-lines -y logBackup.txt logProd.txt > logProd.txt";

/*
 * Fonction qui ajoute le contenu du fichier logDup dans le bon fichier log
 */
void copierFichier(int numServ) {
	char ch;
	FILE* f1 = fopen("logDup.txt","r");
	FILE* f2 = NULL;
	switch (numServ) {
		case 1:
			f2 = fopen("logProd.txt", "a+");
			break;
		case 2:
			f2 = fopen("logBackup.txt", "a+");
			break;
		default:
			f2 = fopen("logInteg.txt", "a+");
			break;
	}
	while ((ch = fgetc(f1)) != EOF) {
		fputc(ch, f2);
	}
	fclose(f1);
	fclose(f2);
}

bool synchroList(){		//Synchronisation si appel effectuée
	FILE * fileProd = fopen("logProd.txt", "r");
	FILE * fileBackup = fopen("logBackup.txt", "r");
	int ch1, ch2;

	if (fileProd == NULL) {
		 exit(1);
	} else if (fileBackup == NULL) {
		 exit(1);
	} else {
		 ch1 = getc(fileProd);
		 ch2 = getc(fileBackup);
}																//On cherche à savoir s'ils sont déjà synchroniser
		 while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2)) {
				ch1 = getc(fileProd);
				ch2 = getc(fileBackup);
		 }

		 if (ch1 == ch2){
				printf("Les fichiers logs sont identiques.\n");
				fclose(fileProd);
				fclose(fileBackup);
				return true;
			}
			else {					//Autrement on renvoie false qui permettra d'appeler copyList
				printf("Les fichiers logs ne sont pas identiques.\nSynchronisation...\n");
				fclose(fileProd);
				fclose(fileBackup);
				sleep(2);
				return false;
	}
}

void copyList(){
	FILE * fileProd = fopen("logProd.txt", "r+");
	FILE * fileBackup = fopen("logBackup.txt", "r+");
	int c=0;
	int semVal;
	sem_getvalue(&ressourceProd, &semVal);
	if (serveur==1) {
		//copy de Prod à Backup
    while((c = fgetc(fileProd) ) != EOF)
    {
        fputc(c, fileBackup);
    }

		fclose(fileProd);
		fclose(fileBackup);
		printf("La synchronisation des logs du serveur Production vers le serveur Backup a bien été effectuée !\n");

	} else {
		//copy de Backup à Prod
    while ((c = fgetc(fileBackup) ) != EOF)
    {
        fputc(c, fileProd);
    }
		fclose(fileProd);
		fclose(fileBackup);
		printf("La synchronisation des logs du serveur Backup vers le serveur Production a bien été effectuée !\n");

	}
}

/*
 * Fonction qui teste si le serveur Production est disponible ou non
 */
void testServer() {
	int semVal;
	sem_getvalue(&ressourceProd, &semVal);
	printf("Recherche d'un serveur disponible...\n");
	sleep(3);
	if (semVal > 0) {
		printf("Serveur production disponible.\nRedirection vers le serveur production...\n");
		serveur = 1;
	} else {
		printf("Serveur production non disponible.\nRedirection vers son serveur backup...\n");
		system(commandPtoB);
		serveur = 2;
		nbErreur++;
	}
}

/*
 * Fonction qui enregistre la ligne de log dans le fichier logDup
 */
void enregistrerLog(int comm, int serv) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	FILE* f = fopen("logDup.txt", "w");
	int sauvegarde_stdout = dup(1);
	dup2(fileno(f), STDOUT_FILENO);
	switch (serv) {
		case 1:
			printf("[%02d-%02d-%d %02d:%02d:%02d] Commande %d effectuée.\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, comm);
			break;
		case 2:
			printf("[%02d-%02d-%d %02d:%02d:%02d] Commande %d effectuée.\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, comm);
			break;
		default:
			if (serveur == 1) {
				printf("[%02d-%02d-%d %02d:%02d:%02d] Commande %d envoyée au serveur production.\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, comm);
			} else {
				printf("[%02d-%02d-%d %02d:%02d:%02d] Commande %d envoyée au serveur backup.\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, comm);
			}
			break;
	}
	close(fileno(f));
	dup2(sauvegarde_stdout, 1);
	close(sauvegarde_stdout);
	fclose(f);
	if (serv != 0) 	copierFichier(0);
	copierFichier(serv);
}

/*
 * Fonction qui enregistre les différentes informations d'une session
 */
void enregistrerStat() {
    FILE *fs = fopen("statsInteg.txt", "a");
    time_t t;
    time(&t);

    if(fs == NULL) {
        printf("Le fichier stats n'existe pas");
    }

    fprintf(fs,"%d\t\t\t\t\t%d\t\t\t\t\t\t%d\t\t\t\t\t\t%d\t\t\t\t%s\n",nbErreur,nbCommEnvoyes,nbCommProd,nbCommBackup,ctime(&t));
    fclose(fs);
}

/*
 * Fonction du thread représentant le serveur Intégration
 */
void *servIntegration(void *arg) {
	int choix;
	printf("Initialisation du serveur d'intégration...\n");
	sleep(3);
	printf("Bienvenue sur le serveur d'intégration. Que souhaitez-vous faire ?\n");
	printf("1.\tEnvoyer une commande\n2.\tSynchroniser les fichiers logs\n3.\tQuitter\n");
	scanf("%d", &choix);
	while (choix != 3) {
		switch (choix) {
			case 1:
				testServer();
				nbCommEnvoyes++;
				enregistrerLog(nbCommEnvoyes, 0);
				if (serveur == 1) {
					sem_post(&sProd); //débloque le serveur Production
				} else {
					sem_post(&sBackup); //débloque le serveur Backup
				}
				sem_wait(&sInteg); //bloque le serveur Intégration
				break;
			case 2:
				if (synchroList()==false){
					copyList();
				}
				break;
			case 3:
				printf("Arrêt du serveur d'intégration...\n");
				sleep(3);
				printf("Au revoir !\n");
				pthread_join(backup, NULL);
				pthread_join(prod, NULL);
				exit(0);
				break;
			default:
				printf("Veuillez choisir une action parmis les choix possibles svp.\n");
				break;
		}
		printf("Que souhaitez-vous faire ?\n");
		printf("1.\tEnvoyer une commande\n2.\tSynchroniser les fichiers logs\n3.\tQuitter\n");
		scanf("%d", &choix);
	}
	return NULL;
}

/*
 * Fonction du thread représentant le serveur Backup
 */
void *servBackup(void *arg) {
	while (1) {
		sem_wait(&sBackup); //bloque le serveur Backup
		printf("Le serveur backup vient d'effectuer la commande %d.\n", nbCommEnvoyes);
		nbCommBackup++;
		enregistrerLog(nbCommEnvoyes, serveur);
		sem_post(&sInteg); //débloque le serveur Intégration
	}
	return NULL;
}

/*
 * Fonction du thread représentant le serveur Production
 */
void *servProduction(void *arg) {
	while (1) {
		sem_wait(&sProd); //bloque le serveur Production
		sem_wait(&ressourceProd); //décrémente les ressources du serveur Production utilisées
		printf("Le serveur production vient d'effectuer la commande %d.\n", nbCommEnvoyes);
		nbCommProd++;
		enregistrerLog(nbCommEnvoyes, serveur);
		sem_post(&sInteg); //débloque le serveur Intégration
	}
	return NULL;
}


int main(int nbarg, char* argv[]) {
	sem_init(&sInteg, 0, 0);
	sem_init(&sProd, 0, 0);
	sem_init(&sBackup, 0, 0);
	sem_init(&ressourceProd, 0, NB_RESSOURCE);
	sem_init(&sTestServer, 0, 0);
	pthread_create(&integ, NULL, servIntegration, NULL);
	pthread_create(&backup, NULL, servBackup, NULL);
	pthread_create(&prod, NULL, servProduction, NULL);
	pthread_join(integ, NULL);
	sem_destroy(&sInteg);
	sem_destroy(&sProd);
	sem_destroy(&sBackup);
	sem_destroy(&ressourceProd);
	sem_destroy(&sTestServer);
	enregistrerStat();
}
