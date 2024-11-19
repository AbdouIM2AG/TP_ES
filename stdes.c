#include "stdes.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>  // Pour vsnprintf et vsscanf

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
static int is_initialized = 0;

void init_es_stdout_stderr() {
    if (is_initialized) return;

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

    is_initialized = 1;  // Marquer comme initialisé
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
int vider(void *f) {
    // Initialisation automatique des fichiers standard si nécessaire
    init_es_stdout_stderr();

    if (!f) {
        write(STDERR_FILENO, "Erreur : fichier non ouvert dans vider\n", 40);
        return -1;
    }

    // Gestion des fichiers standard
    if (f == es_stdout || f == es_stderr) {
        return 0;  // Pas besoin de vidanger es_stdout ou es_stderr
    }

    // Gestion des fichiers FICHIER *
    FICHIER *es_f = (FICHIER *)f;
    if (es_f->mode != 'E') return -1;

    if (es_f->pos > 0) {
        if (write(es_f->fd, es_f->buffer, es_f->pos) == -1) {
            write(STDERR_FILENO, "Erreur lors de la vidange du tampon\n", 36);
            return -1;
        }
        es_f->pos = 0;
    }

    return 0;
}





// Fonction pour écrire des données formatées dans un fichier en utilisant un tampon intermédiaire
int fecriref(void *f, const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Initialisation automatique si nécessaire
    init_es_stdout_stderr();

    if (!f) {
        write(STDERR_FILENO, "Erreur : fichier non ouvert pour fecriref\n", 44);
        va_end(args);
        return -1;
    }

    // Gestion des fichiers standard
    if (f == es_stdout || f == es_stderr) {
        FILE *std_file = (f == es_stdout) ? stdout : stderr;
        vfprintf(std_file, format, args);
        fflush(std_file);
        va_end(args);
        return 0;
    }

    // Gestion des fichiers FICHIER *
    FICHIER *es_f = (FICHIER *)f;
    char buffer[1024];
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (n < 0) {
        write(STDERR_FILENO, "Erreur lors du formatage\n", 26);
        return -1;
    }

    return ecrire(buffer, 1, n, es_f);
}





// Fonction pour lire des données formatées depuis un fichier
int fliref(FICHIER *f, const char *format, ...) {
    if (!f || f->mode != 'L') {
        write(STDERR_FILENO, "Erreur : fichier non ouvert en lecture\n", 40);
        return -1;
    }

    va_list args;
    va_start(args, format);

    // Réinitialisation du tampon
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    // Calcul de la taille des données restantes dans le tampon
    unsigned int remaining = f->size - f->pos;
    if (remaining == 0) {
        remaining = lire(buffer, 1, sizeof(buffer) - 1, f);
        if (remaining <= 0) {
            va_end(args);
            write(STDERR_FILENO, "Erreur : lecture échouée ou fin de fichier\n", 45);
            return -1;
        }
        f->pos = 0;
        f->size = remaining;
    } else {
        memcpy(buffer, f->buffer + f->pos, remaining);
    }

    buffer[remaining] = '\0'; // Terminer la chaîne

    // Analyse avec vsscanf
    va_list args_copy;
    va_copy(args_copy, args);
    int count = vsscanf(buffer, format, args_copy);
    va_end(args_copy);

    if (count <= 0) {
        write(STDERR_FILENO, "Erreur : vsscanf n'a pas pu analyser les données\n", 48);
        va_end(args);
        return -1;
    }

    // Calcul de l'offset à partir du tampon analysé
    char *newline = strchr(buffer, '\n');
    if (newline) {
        int offset = newline - buffer + 1;
        f->pos += offset;  // Réalignement
        if (f->pos >= f->size) {
            f->pos = 0; // Réinitialisation si la position dépasse la taille
            f->size = 0;
        }
    } else {
        f->pos += strlen(buffer); // Ajuster pour le reste du tampon
    }

    va_end(args);
    return count;
}

// Fonction qui écrit directement sur stdout
int ecriref(const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Initialisation automatique si nécessaire
    init_es_stdout_stderr();

    char buffer[1024];
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (n < 0) {
        write(STDERR_FILENO, "Erreur lors du formatage\n", 26);
        return -1;
    }

    write(STDOUT_FILENO, buffer, n);
    return n;
}