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

int has_extension(const char *filename, const char *extension) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;
    return strcmp(dot, extension) == 0;
}


void write_to_statistics_for_bmpFile(const char *filename, BMPFILE *bmpFile, struct stat *fileInfo) {
    int statsFile = open("statistica.txt", O_WRONLY | O_CREAT, 0644);//il deschidem, daca nu e il cream si il facem doar pentru write
    if (statsFile == -1) {
        perror("Eroare la deschiderea fisierului statistica.txt");
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


void process_entry(const char *path) {
    struct stat entryInfo;
    if (lstat(path, &entryInfo) != 0) {
        perror("Error getting entry info");
        return;
    }

    int statsFile = open("statistica.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (statsFile == -1) {
        perror("Error opening statistica.txt");
        return;
    }

    char buffer[1024];
    int length;

    if (S_ISREG(entryInfo.st_mode)) {
        if (has_extension(path, ".bmp")) {
            int file = open_file(path);
            if (file != -1) {
                BMPFILE bmpFile;
                read_from_bmpFile(file, &bmpFile);
                write_to_statistics_for_bmpFile(path, &bmpFile, &entryInfo);
                close(file);
            }
        } else {
            length = sprintf(buffer, "nume fisier: %s\ndimensiune: %lld octeti\nidentificatorul utilizatorului: %d\n",
                             path, (long long)entryInfo.st_size, entryInfo.st_uid);
            write(statsFile, buffer, length);
        }
    } else if (S_ISDIR(entryInfo.st_mode)) {
        char rightsBuffer[10];
        sprintf(rightsBuffer, "%c%c%c%c%c%c",
                (entryInfo.st_mode & S_IRUSR) ? 'R' : '-', 
                (entryInfo.st_mode & S_IWUSR) ? 'W' : '-',
                (entryInfo.st_mode & S_IXUSR) ? 'X' : '-',
                (entryInfo.st_mode & S_IRGRP) ? 'R' : '-', 
                (entryInfo.st_mode & S_IWGRP) ? 'W' : '-',
                (entryInfo.st_mode & S_IXGRP) ? 'X' : '-');

        length = sprintf(buffer, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces: %s\n",
                         path, entryInfo.st_uid, rightsBuffer);
        write(statsFile, buffer, length);
    } else if (S_ISLNK(entryInfo.st_mode)) {
        struct stat targetInfo;
        if (stat(path, &targetInfo) == 0 && S_ISREG(targetInfo.st_mode)) {
            length = sprintf(buffer, "nume legatura: %s\ndimensiune: %lld octeti\ndimensiune fisier: %lld octeti\n",
                             path, (long long)entryInfo.st_size, (long long)targetInfo.st_size);
            write(statsFile, buffer, length);
        }
    }

    close(statsFile);
}


void traverse_directory(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries to avoid infinite recursion
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        process_entry(fullPath);

        // Recursively call traverse_directory if entry is a directory
        struct stat entryInfo;
        if (lstat(fullPath, &entryInfo) != 0) {
            perror("Eroare la obtinerea informatiilor despre intrare");
            continue;
        }

        if (S_ISDIR(entryInfo.st_mode)) {
            traverse_directory(fullPath);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Numar incorect de argumente\n");
        return 1;
    }

    struct stat pathInfo;
    if (lstat(argv[1], &pathInfo) != 0) {
        perror("Eroare la obtinerea informatiilor despre cale");
        return 1;
    }

    if (S_ISDIR(pathInfo.st_mode)) {
        traverse_directory(argv[1]);
    } else {
        process_entry(argv[1]);
    }

    return 0;
}