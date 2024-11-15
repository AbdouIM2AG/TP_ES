#include "stdes.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

// Taille du tampon
#define BUFFER_SIZE 1024

// Structure de gestion des fichiers
struct _ES_FICHIER {
    int fd;                 // Descripteur de fichier
    char buffer[BUFFER_SIZE]; // Tampon statique
    unsigned int pos;       // Position actuelle dans le tampon
    unsigned int size;      // Taille des données dans le tampon
    char mode;              // Mode ('L' pour lecture, 'E' pour écriture)
    int end_of_file;        // Indicateur de fin de fichier
};

// Variables globales pour stdout et stderr
FICHIER *es_stdout = NULL;
FICHIER *es_stderr = NULL;

// Fonction d'initialisation de stdout et stderr
void init_es_stdout_stderr() {
    es_stdout = malloc(sizeof(FICHIER));
    if (!es_stdout) {
        write(STDERR_FILENO, "Erreur : allocation mémoire pour es_stdout\n", 43);
        exit(1);
    }
    es_stdout->fd = STDOUT_FILENO;
    es_stdout->pos = 0;
    es_stdout->size = 0;
    es_stdout->mode = 'E';
    es_stdout->end_of_file = 0;

    es_stderr = malloc(sizeof(FICHIER));
    if (!es_stderr) {
        write(STDERR_FILENO, "Erreur : allocation mémoire pour es_stderr\n", 43);
        exit(1);
    }
    es_stderr->fd = STDERR_FILENO;
    es_stderr->pos = 0;
    es_stderr->size = 0;
    es_stderr->mode = 'E';
    es_stderr->end_of_file = 0;
}

// Fonction pour ouvrir un fichier
FICHIER *ouvrir(const char *nom, char mode) {
    FICHIER *file = malloc(sizeof(FICHIER));
    if (!file) {
        write(STDERR_FILENO, "Erreur : allocation mémoire échouée\n", 37);
        return NULL;
    }

    file->pos = 0;
    file->size = 0;
    file->end_of_file = 0;

    if (mode == 'L') {
        file->fd = open(nom, O_RDONLY);
        file->mode = 'L';
    } else if (mode == 'E') {
        file->fd = open(nom, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        file->mode = 'E';
    } else {
        write(STDERR_FILENO, "Erreur : mode inconnu\n", 23);
        free(file);
        return NULL;
    }

    if (file->fd == -1) {
        write(STDERR_FILENO, "Erreur : impossible d'ouvrir le fichier\n", 41);
        free(file);
        return NULL;
    }

    return file;
}

// Fonction pour fermer un fichier
int fermer(FICHIER *f) {
    if (!f) return -1;

    if (f->mode == 'E' && f->pos > 0) {
        write(f->fd, f->buffer, f->pos);  // Vider le tampon
    }

    close(f->fd);
    free(f);
    return 0;
}

// Fonction pour lire dans un fichier
int lire(void *p, unsigned int taille, unsigned int nbelem, FICHIER *f) {
    if (!f || f->fd == -1) {
        write(STDERR_FILENO, "Erreur : pointeur fichier NULL ou fichier non ouvert dans lire\n", 62);
        return -1;
    }
    if (f->mode != 'L') return -1;

    unsigned int total_bytes = taille * nbelem;
    unsigned int bytes_read = 0;

    char *dest = (char *)p;

    while (bytes_read < total_bytes) {
        if (f->pos >= f->size) {
            // Remplir le tampon
            ssize_t read_bytes = read(f->fd, f->buffer, BUFFER_SIZE);
            if (read_bytes <= 0) {
                f->end_of_file = 1;
                break;
            }
            f->pos = 0;
            f->size = read_bytes;
        }

        unsigned int chunk_size = total_bytes - bytes_read;
        if (chunk_size > f->size - f->pos) {
            chunk_size = f->size - f->pos;
        }

        memcpy(dest + bytes_read, f->buffer + f->pos, chunk_size);
        f->pos += chunk_size;
        bytes_read += chunk_size;
    }

    return bytes_read / taille;
}

// Fonction pour écrire dans un fichier
int ecrire(const void *p, unsigned int taille, unsigned int nbelem, FICHIER *f) {
    if (!f || f->fd == -1) {
        write(STDERR_FILENO, "Erreur : pointeur fichier NULL ou fichier non ouvert dans ecrire\n", 64);
        return -1;
    }
    if (f->mode != 'E') return -1;

    unsigned int total_bytes = taille * nbelem;
    unsigned int bytes_written = 0;

    const char *src = (const char *)p;

    while (bytes_written < total_bytes) {
        if (f->pos == BUFFER_SIZE) {
            // Écrire le tampon
            if (write(f->fd, f->buffer, BUFFER_SIZE) == -1) {
                write(STDERR_FILENO, "Erreur lors de l'écriture dans le fichier\n", 43);
                return -1;
            }
            f->pos = 0;
        }

        unsigned int chunk_size = total_bytes - bytes_written;
        if (chunk_size > BUFFER_SIZE - f->pos) {
            chunk_size = BUFFER_SIZE - f->pos;
        }

        memcpy(f->buffer + f->pos, src + bytes_written, chunk_size);
        f->pos += chunk_size;
        bytes_written += chunk_size;
    }

    return bytes_written / taille;
}

// Fonction pour vider un tampon
int vider(FICHIER *f) {
    if (!f || f->fd == -1) {
        write(STDERR_FILENO, "Erreur : pointeur fichier NULL ou fichier non ouvert dans vider\n", 62);
        return -1;
    }
    if (f->mode != 'E') return -1;

    if (f->pos > 0) {
        if (write(f->fd, f->buffer, f->pos) == -1) {
            write(STDERR_FILENO, "Erreur lors de la vidange du tampon\n", 36);
            return -1;
        }
        f->pos = 0;
    }

    return 0;
}
