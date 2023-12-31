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
#include <libgen.h>
#include <sys/wait.h>



/* Noi când citim din fișier trebuie să citim exact atâția bytes din fișier. Cu verde trebuie
   să ajungem la file size, trebuie să citim 2 bytes pe care să îi ignorăm, iar după iar ignorăm 
   iarăși 12 bytes și dăm de width și height. */

#define BUFSIZ 1024

typedef struct {
    int line_count;
    char output_file[1024];
    char input_file[1024]; // Numele fișierului de intrare procesat
} ProcessInfo;

typedef struct {
    pid_t pid;
    int exitCode;
    int lineCount;
} ChildProcessData;


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


void execute_shell_script(const char *script_path, const char *data) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[1]); 
        dup2(pipefd[0], STDIN_FILENO); 
        execlp("bash", "bash", script_path, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        close(pipefd[0]);
        write(pipefd[1], data, strlen(data));
        close(pipefd[1]);
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
}

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


pid_t execute_script_with_content(const char *script_path, const char *data, char c) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[1]); 
        dup2(pipefd[0], STDIN_FILENO);

        char c_str[2] = {c, '\0'};
        execlp("bash", "bash", script_path, c_str, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        close(pipefd[0]);
        write(pipefd[1], data, strlen(data));
        close(pipefd[1]);
    }
    return pid; 
}


void write_to_statistics_for_bmpFile(const char *output_path, const char *filename, BMPFILE *bmpFile, struct stat *fileInfo, int *line_count) {
    int statsFile = open(output_path, O_WRONLY | O_CREAT, 0644);//il deschidem, daca nu e il cream si il facem doar pentru write
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
     *line_count += 10;
    close(statsFile);
}

void convert_to_grayscale(const char *file_path) {
    int fd = open(file_path, O_RDWR);
    if (fd == -1) {
        perror("Eroare la deschiderea fișierului pentru conversia in tonuri de gri");
        return;
    }

    BMPFILE bmpFile;
    read_from_bmpFile(fd, &bmpFile); 

    lseek(fd, bmpFile.dataOffset, SEEK_SET); 

    unsigned char pixel[3]; 
    for (unsigned int i = 0; i < bmpFile.height; i++) {
        for (unsigned int j = 0; j < bmpFile.width; j++) {
            read(fd, pixel, 3); 
            unsigned char gray = (unsigned char)(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);
            pixel[0] = pixel[1] = pixel[2] = gray;
            lseek(fd, -3, SEEK_CUR);
            write(fd, pixel, 3);
        }
    }

    close(fd);
}

void process_entry(const char *input_path, const char *output_dir,int *pfd,char caracter,int *num_children) {
    int line_count=0;
    struct stat entryInfo;
    if (lstat(input_path, &entryInfo) != 0) {
        perror("Eroare la obținerea informațiilor de intrare");
        return;
    }

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%s/%s_statistica.txt", output_dir, basename(strdup(input_path)));

    pid_t pid = fork();
    if (pid == 0) { // Procesul fiu
        int statsFile = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (statsFile == -1) {
            perror("eroare la deschiderea fisierului de statistica");
            exit(1);
        }

        char buffer[1024];
        int length;
        
          if (S_ISREG(entryInfo.st_mode) && !has_extension(input_path, ".bmp")) {
            int file = open_file(input_path);
            if (file != -1) {
                char file_content[BUFSIZ];
                ssize_t read_bytes = read(file, file_content, sizeof(file_content) - 1);
                if (read_bytes > 0) {
                    file_content[read_bytes] = '\0';
                    execute_script_with_content("script.sh", file_content, caracter);
                    length = sprintf(buffer, "nume fisier: %s\ndimensiune: %lld octeti\nidentificatorul utilizatorului: %d\n",
                                 input_path, (long long)entryInfo.st_size, entryInfo.st_uid);
                                  line_count = 3;
                write(statsFile, buffer, length);
                }
                close(file);
            

            } else {
                
                    char file_content[BUFSIZ];
                    if (file != -1) {
                        ssize_t read_bytes = read(file, file_content, sizeof(file_content) - 1);
                        if (read_bytes > 0) {
                            file_content[read_bytes] = '\0';
                            printf("Conținutul citit: %s\n", file_content); 
                            execute_script_with_content("script.sh", file_content, caracter);
                        }
                        close(file);
                        }

        
                pid_t pid_script = fork();
                if (pid_script == 0) { 
                    int pipe_script[2];
                    if (pipe(pipe_script) == -1) {
                        perror("Pipe failed");
                        exit(1);
                    }
                    write(pipe_script[1], file_content, strlen(file_content));
                    close(pipe_script[1]); 

                 
                    dup2(pipe_script[0], STDIN_FILENO);
                    char character_str[2] = {caracter, '\0'};
                    execlp("bash", "bash", "./script.sh", character_str, NULL);
                    perror("execlp failed");
                    exit(EXIT_FAILURE);
                } else if (pid_script > 0) {
                    int status_script;
                    waitpid(pid_script, &status_script, 0);
                    if (WIFEXITED(status_script)) {
                        int count = WEXITSTATUS(status_script);
                        printf("nr de propozitii corecte pentru fisierul '%s': este %d\n", input_path, count);
                    }
                } else {
                    perror("Fork failed");
                }
                close(file);
            }
        } else if (S_ISREG(entryInfo.st_mode) && has_extension(input_path, ".bmp")) { {
                BMPFILE bmpFile;
                int file = open_file(input_path);
                read_from_bmpFile(file, &bmpFile);
                line_count = 0;
                write_to_statistics_for_bmpFile(output_path, input_path, &bmpFile, &entryInfo, &line_count);
                close(file);
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
                             input_path, entryInfo.st_uid, rightsBuffer);
            write(statsFile, buffer, length);
            line_count=3;
        } else if (S_ISLNK(entryInfo.st_mode)) {
            char targetPath[1024];
            ssize_t len = readlink(input_path, targetPath, sizeof(targetPath) - 1);
            if (len != -1) {
                targetPath[len] = '\0';
                length = sprintf(buffer, "nume legatura: %s -> %s\n", input_path, targetPath);
                write(statsFile, buffer, length);
                line_count=1;
            } else {
                length = sprintf(buffer, "nume legatura: %s -> (eroare la citire)\n", input_path);
                write(statsFile, buffer, length);
                line_count=1;
            }
        }
     
         ChildProcessData data;
        data.pid = getpid();
        data.exitCode = 0;
        data.lineCount = line_count;
        close(pfd[0]); 
        write(pfd[1], &data, sizeof(data));
        close(pfd[1]); 

        exit(0);
    } else if (pid > 0) {
         ChildProcessData data;
        close(pfd[1]); 
        read(pfd[0], &data, sizeof(data));
        close(pfd[0]); 

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            data.exitCode = WEXITSTATUS(status);
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", data.pid, data.exitCode);

            char statsPath[1024];
            snprintf(statsPath, sizeof(statsPath), "%s/statistica.txt", output_dir);
            int statsFile = open(statsPath, O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (statsFile != -1) {
                char buffer[1024];
                int length = sprintf(buffer, "Procesul cu pid-ul %d s-a incheiat cu codul %d si a scris %d linii\n", 
                                     data.pid, data.exitCode, data.lineCount);
                write(statsFile, buffer, length);
                close(statsFile);
            }
        }
    } else {
        perror("Fork failed");
    }
}

void traverse_directory(const char *input_dir, const char *output_dir,char caracter) {
    DIR *dir = opendir(input_dir);
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului");
        return;
    }

    struct dirent *entry;
    int num_children = 0;
    int pfd[2 * BUFSIZ]; 

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", input_dir, entry->d_name);

        if (pipe(pfd + num_children * 2) == -1) {
            perror("Eroare la crearea pipe-ului");
            continue;
        }

        process_entry(fullPath, output_dir, pfd + num_children * 2,caracter,&num_children);
        close(pfd[num_children * 2 + 1]); 
        num_children++;
    }

   for (int i = 0; i < num_children; i++) {
    ProcessInfo info;
    int status;
    pid_t pid = wait(&status);
    if (pid > 0 && WIFEXITED(status)) {
        read(pfd[i * 2], &info, sizeof(info));
        close(pfd[i * 2]); 

        char statsPath[1024];
        snprintf(statsPath, sizeof(statsPath), "%s/statistica.txt", output_dir);
        int statsFile = open(statsPath, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (statsFile == -1) {
            perror("Eroare la deschiderea fișierului de statistici pentru scriere");
            continue;
        }

        char buffer[1024];
        int length = sprintf(buffer, "Procesul cu pid-ul %d s-a incheiat si a scris %d linii in %s pentru fisierul %s\n", 
                             pid, info.line_count, info.output_file, info.input_file);
        write(statsFile, buffer, length);

        close(statsFile);
    }
}
}

int main(int argc, char *argv[]) {

    int num_children = 0;
    if (argc != 4) {
        printf("Numar incorect de argumente\n");
        return 1;
    }

    char c = argv[3][0];
    if (!isalnum(c)) {
        printf("Caracterul trebuie sa fie alfanumeric\n");
        return 1;
    }

    struct stat pathInfo;
    if (lstat(argv[1], &pathInfo) != 0) {
        perror("Eroare la obtinerea informatiilor despre cale");
        return 1;
    }
    mkdir(argv[2], 0777);

    if (S_ISDIR(pathInfo.st_mode)) {
        traverse_directory(argv[1], argv[2],c);
    } else {
        int pfd[2]; 
        if (pipe(pfd) == -1) {
            perror("Eroare la crearea pipe-ului");
            return 1;
        }
        process_entry(argv[1], argv[2], pfd,c,&num_children);
        close(pfd[1]); 
        close(pfd[0]); 
    }

    char *data_to_send = "a\nThis is a test sentence. Another one? Yes!";
    execute_shell_script("./script.sh", data_to_send);

    return 0;
}
