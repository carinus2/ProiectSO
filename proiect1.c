#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
/* Noi când citim din fișier trebuie să citim exact atâția bytes din fișier. Cu verde trebuie
   să ajungem la file size, trebuie să citim 2 bytes pe care să îi ignorăm, iar după iar ignorăm 
   iarăși 12 bytes și dăm de width și height. */

#define BUFSIZ 1024

typedef struct {
    short int signature; // 2 bytes
    int fileSize; // 4 bytes
    int reserved; // 4 bytes
    int dataOffset; // 4 bytes
    int size; // 4 bytes
    unsigned int width; // 4 bytes
    unsigned int height; // 4 bytes
    short int planes; // 2 bytes
    short int bitCount; // 2 bytes
    int compression; // 4 bytes
    unsigned int imageSize; // 4 bytes
    unsigned int xPixelsPerM; // 4 bytes
    unsigned int yPixelsPerM; // 4 bytes
    unsigned int colorsUsed; // 4 bytes
    unsigned int colorsImportant; // 4 bytes
} BMPFILE;



void read_from_bmpFile(int fd, BMPFILE *file){
    lseek(fd, 0, SEEK_SET); 
    read(fd, &(file->signature), 2);
    read(fd, &(file->fileSize), 4);
    read(fd, &(file->reserved), 4);
    read(fd, &(file->dataOffset), 4);
    read(fd, &(file->size), 4);
    read(fd, &(file->width), 4);
    read(fd, &(file->height), 4);
    read(fd, &(file->planes), 2);
    read(fd, &(file->bitCount), 2);
    read(fd, &(file->compression), 4);
    read(fd, &(file->imageSize), 4);
    read(fd, &(file->xPixelsPerM), 4);
    read(fd, &(file->yPixelsPerM), 4);
    read(fd, &(file->colorsUsed), 4);
    read(fd, &(file->colorsImportant), 4);
}

int is_director(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        perror("Eroare la apelul stat");
        return 0;
    }
    return S_ISDIR(info.st_mode);
}

int open_director(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return 0;
    }

    struct dirent *in = readdir(dir);
    closedir(dir);
    return in != NULL;
}

int open_file(const char *path){
    int fileDescriptor = open(path, O_RDONLY);
    if (fileDescriptor == -1) {
        perror("Eroare la deschiderea fișierului");
    }
    return fileDescriptor;
}

void write_to_statistica(const char *filename, BMPFILE *bmpFile, struct stat *fileInfo) {
    int statsFile = open("statistica.txt", O_WRONLY | O_CREAT, 0644);//il deschidem, daca nu e il cream si il facem doar pentru write
    if (statsFile == -1) {
        perror("Error opening statistica.txt");
        return;
    }

    char buffer[1024]; 
    char modTimeStr[20]; 
    strftime(modTimeStr, 20, "%d.%m.%Y", localtime(&fileInfo->st_mtime));

    int length;

    length = sprintf(buffer, "nume fisier: %s\n", filename);
    write(statsFile, buffer, length);

    
    length = sprintf(buffer, "inaltime: %u\n", bmpFile->height);
    write(statsFile, buffer, length);

    length = sprintf(buffer, "lungime: %u\n", bmpFile->width);
    write(statsFile, buffer, length);
    
    length = sprintf(buffer, "dimensiune: %lld octeti\n", (long long)fileInfo->st_size);
    write(statsFile, buffer, length);

    length = sprintf(buffer, "identificatorul utilizatorului: %d\n", fileInfo->st_uid);
    write(statsFile, buffer, length);

    length = sprintf(buffer, "timpul ultimei modificari: %s\n", modTimeStr);
    write(statsFile, buffer, length);

    length = sprintf(buffer, "contorul de legaturi: %lu\n", (unsigned long)fileInfo->st_nlink);
    write(statsFile, buffer, length);

    length = sprintf(buffer, "drepturi de acces user: %c%c%c\n", 
                    (fileInfo->st_mode & S_IRUSR) ? 'R' : '-', 
                    (fileInfo->st_mode & S_IWUSR) ? 'W' : '-',
                    (fileInfo->st_mode & S_IXUSR) ? 'X' : '-');
    write(statsFile, buffer, length);

    length = sprintf(buffer, "drepturi de acces grup: %c%c%c\n", 
                    (fileInfo->st_mode & S_IRGRP) ? 'R' : '-', 
                    (fileInfo->st_mode & S_IWGRP) ? 'W' : '-',
                    (fileInfo->st_mode & S_IXGRP) ? 'X' : '-');
    write(statsFile, buffer, length);

    length = sprintf(buffer, "drepturi de acces altii: %c%c%c\n", 
                    (fileInfo->st_mode & S_IROTH) ? 'R' : '-', 
                    (fileInfo->st_mode & S_IWOTH) ? 'W' : '-',
                    (fileInfo->st_mode & S_IXOTH) ? 'X' : '-');
    write(statsFile, buffer, length);

    close(statsFile);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Numar incorect de argumente\n");
        return 1;
    }

    if (is_director(argv[1])) {
        if (open_director(argv[1])) {
            printf("%s este un director și conține fișiere sau directoare.\n", argv[1]);
        } else {
            printf("%s nu se pot citi intrările din director sau este gol.\n", argv[1]);
        }
        return 0;
    }

    int file = open_file(argv[1]);
    if (file == -1) {
        return 1;
    }

    BMPFILE bmpFile;
    read_from_bmpFile(file, &bmpFile);

    struct stat fileInfo;
    if (stat(argv[1], &fileInfo) != 0) {
        perror("eroare la primirea informatiilor despre fisier");
        return 1;
    }

    write_to_statistica(argv[1], &bmpFile, &fileInfo);

    close(file); 

    return 0;
}