#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;

int este_director(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        perror("Eroare la apelul stat");
        return 0;
    }
    return S_ISDIR(info.st_mode);
}

int poate_fi_deschis(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return 0;
    }

    struct dirent *in = readdir(dir);
    closedir(dir);
    return in != NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Numar incorect de argumente\n");
        return 1;
    }

    if (este_director(argv[1])) {
        if (poate_fi_deschis(argv[1])) {
            printf("%s este un director și conține fișiere sau directoare.\n", argv[1]);
        } else {
            printf("%s nu se pot citi intrările din director sau este gol.\n", argv[1]);
        }
    } else {
        printf("%s nu este un director.\n", argv[1]);
    }

    return 0;
}