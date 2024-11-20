#define _POSIX_C_SOURCE 199309L
#include "stdes.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#ifdef NEED_LINUX_TIME
#include <linux/time.h>
#endif

int main(int argc, char *argv[]) {
    FICHIER *f1, *f2; // Pointeurs pour les fichiers
    char buffer[32768]; // Tampon de 1024 octets
    int bytes_read;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./test_buffered <source_file> <destination_file>\n", 57);
        exit(-1);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Initialiser es_stdout et es_stderr
    init_es_stdout_stderr();

    // Ouvrir les fichiers
    f1 = ouvrir(argv[1], 'L');
    if (!f1) {
        write(STDERR_FILENO, "Erreur : impossible d'ouvrir le fichier source\n", 47);
        exit(-1);
    }

    f2 = ouvrir(argv[2], 'E');
    if (!f2) {
        write(STDERR_FILENO, "Erreur : impossible d'ouvrir le fichier destination\n", 53);
        fermer(f1);
        exit(-1);
    }

    // Lire et écrire le contenu
    while ((bytes_read = lire(buffer, 1, sizeof(buffer), f1)) > 0) {
        if (ecrire(buffer, 1, bytes_read, f2) == -1) {
            write(STDERR_FILENO, "Erreur : écriture échouée dans le fichier destination\n", 55);
            fermer(f1);
            fermer(f2);
            exit(-1);
        }
    }

    // Fermer les fichiers
    fermer(f1);
    fermer(f2);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Temps d'exécution (E/S tamponnées) : %f secondes\n", elapsed_time);

    return 0;
}
