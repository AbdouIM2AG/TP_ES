#include "stdes.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

struct _ES_FICHIER {
    int fd;                 // Descripteur de fichier
    char *buffer;           // Tampon
    size_t buffer_size;     // Taille du tampon
    size_t pos;             // Position actuelle dans le tampon
    int mode;               // Mode ('L' pour lecture, 'E' pour écriture)
    int end_of_file;        // Indicateur de fin de fichier
};

// Variables stdout et stderr
FICHIER *es_stdout = NULL;
FICHIER *es_stderr = NULL;

FICHIER *ouvrir(const char *nom, char mode) {
    FICHIER *file = malloc(sizeof(FICHIER));
    if (!file) return NULL;

    file->buffer = malloc(BUFFER_SIZE);
    if (!file->buffer) {
        free(file);
        return NULL;
    }

    file->buffer_size = BUFFER_SIZE;
    file->pos = 0;
    file->end_of_file = 0;

    if (mode == 'L') {
        file->fd = open(nom, O_RDONLY);
        file->mode = 'L';
    } else if (mode == 'E') {
        file->fd = open(nom, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        file->mode = 'E';
    } else {
        free(file->buffer);
        free(file);
        return NULL;
    }

    if (file->fd == -1) {
        free(file->buffer);
        free(file);
        return NULL;
    }

    return file;
}

int fermer(FICHIER *f) {
    if (!f) return -1;

    if (f->mode == 'E' && f->pos > 0) {
        write(f->fd, f->buffer, f->pos);  // Vider le tampon
    }

    close(f->fd);
    free(f->buffer);
    free(f);
    return 0;
}

int lire(void *p, unsigned int taille, unsigned int nbelem, FICHIER *f) {
    if (f->mode != 'L' || f->end_of_file) return 0;

    size_t total_bytes = taille * nbelem;
    size_t bytes_read = 0;

    while (bytes_read < total_bytes) {
        if (f->pos >= f->buffer_size) {
            ssize_t read_bytes = read(f->fd, f->buffer, BUFFER_SIZE);
            if (read_bytes <= 0) {
                f->end_of_file = 1;
                break;
            }
            f->pos = 0;
            f->buffer_size = read_bytes;
        }

        size_t bytes_to_copy = total_bytes - bytes_read;
        if (bytes_to_copy > f->buffer_size - f->pos) {
            bytes_to_copy = f->buffer_size - f->pos;
        }

        memcpy((char *)p + bytes_read, f->buffer + f->pos, bytes_to_copy);
        f->pos += bytes_to_copy;
        bytes_read += bytes_to_copy;
    }

    return bytes_read / taille;
}

int ecrire(const void *p, unsigned int taille, unsigned int nbelem, FICHIER *f) {
    if (f->mode != 'E') return -1;

    size_t total_bytes = taille * nbelem;
    size_t bytes_written = 0;

    while (bytes_written < total_bytes) {
        if (f->pos == BUFFER_SIZE) {
            write(f->fd, f->buffer, BUFFER_SIZE);
            f->pos = 0;
        }

        size_t bytes_to_copy = total_bytes - bytes_written;
        if (bytes_to_copy > BUFFER_SIZE - f->pos) {
            bytes_to_copy = BUFFER_SIZE - f->pos;
        }

        memcpy(f->buffer + f->pos, (char *)p + bytes_written, bytes_to_copy);
        f->pos += bytes_to_copy;
        bytes_written += bytes_to_copy;
    }

    return bytes_written / taille;
}

int vider(FICHIER *f) {
    if (f->mode != 'E') return -1;
    if (f->pos > 0) {
        write(f->fd, f->buffer, f->pos);
        f->pos = 0;
    }
    return 0;
}

