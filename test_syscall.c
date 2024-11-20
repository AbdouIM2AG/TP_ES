#define _POSIX_C_SOURCE 199309L
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
//#include <linux/time.h>
#ifdef NEED_LINUX_TIME
#include <linux/time.h>
#endif

int main(int argc, char *argv[]) {
    int f1, f2; // Descripteurs pour les fichiers
    char buffer[1024]; // Tampon de 1024 octets
    ssize_t bytes_read, bytes_written;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./test_syscall <source_file> <destination_file>\n", 55);
        exit(-1);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Ouvrir les fichiers
    f1 = open(argv[1], O_RDONLY);
    if (f1 == -1) {
        perror("Erreur : impossible d'ouvrir le fichier source");
        exit(-1);
    }

    f2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f2 == -1) {
        perror("Erreur : impossible d'ouvrir le fichier destination");
        close(f1);
        exit(-1);
    }

    // Lire et écrire le contenu
    while ((bytes_read = read(f1, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(f2, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Erreur : écriture partielle dans le fichier destination");
            close(f1);
            close(f2);
            exit(-1);
        }
    }

    if (bytes_read == -1) {
        perror("Erreur : lecture échouée");
    }

    // Fermer les fichiers
    close(f1);
    close(f2);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Temps d'exécution (appels système) : %f secondes\n", elapsed_time);

    return 0;
}
