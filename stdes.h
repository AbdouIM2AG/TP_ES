#ifndef _STDES_H
#define _STDES_H

#include<stddef.h>
#include<sys/types.h>

// Définit la taille du tampon
#define BUFFER_SIZE 1024 

// Structure opaque pour le type FICHIER
struct _ES_FICHIER;
typedef struct _ES_FICHIER FICHIER;

// Déclarations des variables stdout et stderr
extern FICHIER *es_stdout;
extern FICHIER *es_stderr;

/* mode: 'L' = lecture, 'E' = écriture */
FICHIER *ouvrir(const char *nom, char mode);
int fermer(FICHIER*f);
int lire(void *p, unsigned int taille, unsigned int nbelem, FICHIER *f);
int ecrire(const void *p, unsigned int taille, unsigned int nbelem, FICHIER *f);
int vider(FICHIER *f);

int fecriref (FICHIER *f, const char *format, ...);
/* directly in stdout */
int ecriref (const char *format, ...);
int fliref (FICHIER *f, const char *format, ...);

#endif

