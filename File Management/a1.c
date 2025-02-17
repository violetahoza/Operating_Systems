#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MIN_VERSION 55
#define MAX_VERSION 171

typedef struct
{
    char magic[3];
    int size, version, nr_sections;
} Header; // represent the header of a SF file

typedef struct
{
    char name[11];
    char type; // because I want to read a single byte
    int offset, size;
} SectionHeader; // represent the header of a section in the SF file

int checkPermissions(mode_t mode, char *perm); // checks file permissions against the perm string
void listDirectory(char *path, char *filtering_options); // lists the contents of a directory
void listRec(char *basePath, char *filtering_options); // lists the contents of a directory, but recursively
int parseSF(char *path, int print); // verifies if the file is a SF file, and prints its header's information if print = 1
void findSF(char *path); // finds SF files within a directory, that have at least one section with exactly 16 lines
void extractLine(char *path, int sectionNr, int lineNr); // extract a specific line from a specific section of the SF file

int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            printf("71131\n");
        }
        else if (strcmp(argv[1], "list") == 0) // if the command is "list"
        {
            char *path = NULL;
            int recursive = 0;
            char *filtering_options = NULL;

            for (int i = 2; i < argc; i++) // iterate through the command-line arguments
            {
                if (strncmp(argv[i], "path=", 5) == 0) // check for the format path=<dir_path>
                {
                    path = (char *)malloc((strlen(argv[i]) - 4) * sizeof(char)); // allocate memory for the path
                    strcpy(path, argv[i] + 5); // copy the path from the argument
                }
                else if (strncmp(argv[i], "recursive", 9) == 0) // check if one of the arguments is recursive
                    recursive = 1; 
                else
                {
                    filtering_options = (char *)malloc((strlen(argv[i])) * sizeof(char)); // allocate memory for the filtering options
                    strcpy(filtering_options, argv[i]); // copy filtering options
                }
            }
            if (recursive == 0)
                listDirectory(path, filtering_options);
            else if (recursive == 1)
            {
                printf("SUCCESS\n");
                listRec(path, filtering_options);
            }
            free(path);
            free(filtering_options);
        }
        else if (strcmp(argv[1], "parse") == 0) // if the command is "parse"
        {
            if (argc != 3) // check if the correct nr of arguments is provided
            {
                printf("missing file path\n");
                return -1;
            }
            if (strncmp(argv[2], "path=", 5) == 0) // check if the argument format is correct
            {
                char *path = (char *)malloc((strlen(argv[2]) - 4) * sizeof(char));
                strcpy(path, argv[2] + 5);
                parseSF(path, 1);
                free(path);
            }
        }
        else if (strcmp(argv[1], "extract") == 0) // check if the command is "extract"
        {
            if (argc != 5)
            {
                perror("invalid nr of arguments");
                return -1;
            }
            char *path = NULL;
            int sectionNr = 0, lineNr = 0;
            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    path = (char *)malloc((strlen(argv[i]) - 4) * sizeof(char));
                    strcpy(path, argv[i] + 5);
                }
                else if (strncmp(argv[i], "line=", 5) == 0)
                    lineNr = atoi(argv[i] + 5); // get the line number by conversion from string to integer
                else if (strncmp(argv[i], "section=", 8) == 0)
                    sectionNr = atoi(argv[i] + 8); // get the section number by conversion from string to integer
            }
            extractLine(path, sectionNr, lineNr);
            free(path);
        }
        else if (strcmp(argv[1], "findall") == 0) // if the command is "findall"
        {
            if (argc != 3)
            {
                perror("invalid nr of arguments");
                return -1;
            }
            char *path;
            if (strncmp(argv[2], "path=", 5) == 0)
            {
                path = (char *)malloc((strlen(argv[2]) - 4) * sizeof(char));
                strcpy(path, argv[2] + 5);
            }
            // printf("%s", path);
            printf("SUCCESS\n");
            findSF(path);
            free(path);
        }
    }
    return 0;
}

void extractLine(char *path, int sectionNr, int lineNr)
{
    if (!parseSF(path, 0)) // check if the file respects the format of a SF file
    {
        printf("ERROR\n invalid file");
        return;
    }

    int file = open(path, O_RDONLY); // open the SF file for reading
    if (file == -1)
    {
        perror("Cannot open the file\n");
        return;
    }

    Header *header = (Header *)malloc(sizeof(Header));
    lseek(file, 5, SEEK_SET);
    read(file, &header->nr_sections, 1); // get the number of sections
    if (header->nr_sections < sectionNr) // check if the specified section number is valid
    {
        printf("ERROR\n invalid section");
        return;
    }

    SectionHeader section;

    lseek(file, 6 + 19 * (sectionNr - 1), SEEK_SET); // go to the specified section's header (skip 6 bytes from the haeder + 19 bytes per section header, considering that the section headers are consecutive)
    lseek(file, 11, SEEK_CUR); // skip the section's name and type
    // read(file, section.name, 10);
    // section.name[10] = '\0';
    // read(file, &section.type, 1);
    read(file, &section.offset, 4); // get the offset
    read(file, &section.size, 4); //and the section size

    char *content = (char *)malloc(section.size * sizeof(char)); // allocate memory for the content of the section
    char *line = (char *)malloc(section.size * sizeof(char)); // allocate memory for the wanted line
    lseek(file, section.offset, SEEK_SET); // go to the start of the specified section (which we know it starts from the byte at offset)
    read(file, content, section.size); // read the content of the section

    int lineCnt = 0, cnt = 0, j = 0;
    for (j = 0; j < section.size; j++) // go through the section content to find the specified line
    {
        if ((int)content[j] == 10) // if we found the end of the line
        {
            lineCnt++;
            if (lineCnt == lineNr)
            {
                for (int k = j - 1; k >= 0; k--) // if the specified line is found, extract it in reverse order
                {
                    if ((int)content[k] != 10)
                    {
                        line[cnt++] = content[k];
                        //printf("%c ", content[k]);
                    }
                    else
                        break;
                }
                break; // if we found the specified line, we can stop
            }
        }
    }
    lineCnt++;
    // printf("%d ", lineCnt);
    if (lineCnt == lineNr)
    {
        for (int k = j - 1; k >= 0; k--)
        {
            if ((int)content[k] != 10)
                line[cnt++] = content[k];
            else
                break;
        }
    }
    line[cnt++] = '\0';
    if (lineCnt < lineNr)
    {
        printf("ERROR\ninvalid line");
        return;
    } else {
        printf("SUCCESS\n");
        printf("%s", line);
    }

    free(header);
    free(line);
    free(content);
}

void findSF(char *path)
{
    DIR *dir = NULL;
    dir = opendir(path);
    struct dirent *entry = NULL;
    struct stat statbuf;

    if (dir == NULL)
    {
        printf("ERROR\ninvalid directory path");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) // go through each entry in the directory
    {
        char* fullPath = calloc(1024, sizeof(char));
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            snprintf(fullPath, sizeof(path) + sizeof(entry->d_name) + 1, "%s/%s", path, entry->d_name); // construct the full path of the entry

            if (lstat(fullPath, &statbuf) == 0) // get information about the entry
            {
                if (S_ISDIR(statbuf.st_mode))
                    findSF(fullPath);
                else if (S_ISREG(statbuf.st_mode)) // if the entry is a regular file
                {
                    if (parseSF(fullPath, 0) == 1) // if the file is a SF file
                    {
                        int file = open(fullPath, O_RDONLY); // open the file for reading
                        if (file == -1)
                        {
                            printf("ERROR\ninvalid directory path");
                            close(file);
                            closedir(dir);
                            free(fullPath);
                            return;
                        }

                        Header *header = (Header *)malloc(sizeof(Header));
                        lseek(file, 5, SEEK_SET); // skip the magic nr, header size and version
                        read(file, &header->nr_sections, 1); // read the nr of sections from the file

                        SectionHeader *sections = (SectionHeader *)malloc(header->nr_sections * sizeof(SectionHeader));
                        for (int i = 0; i < header->nr_sections; i++)
                        {
                            lseek(file, 6 + 19 * i + 11, SEEK_SET);
                            read(file, &sections[i].offset, 4);
                            read(file, &sections[i].size, 4);

                            char *content = (char *)malloc(sections[i].size * sizeof(char));
                            lseek(file, sections[i].offset, SEEK_SET);
                            read(file, content, sections[i].size);

                            int lineIndex = 0;
                            for (int j = 0; j < sections[i].size; j++)
                            {
                                if ((int)content[j] == 10)
                                {
                                    lineIndex++; // count the nr of lines in the section content
                                }
                            }
                            lineIndex++;
                            // printf("%d ", lineIndex);
                            if (lineIndex == 16)
                            {
                                printf("%s\n", fullPath);
                                free(content);
                                break; // if we found a section with 16 lines, we can skip the others
                            }
                            free(content);
                        }
                        free(sections);
                        free(header);
                        close(file);
                    }
                }
            }
        }
        free(fullPath);
    }
    closedir(dir);
}

int parseSF(char *path, int print)
{
    int file = open(path, O_RDONLY); // open the file for reading
    if (file == -1)
    {
        perror("Cannot open the file\n");
        close(file);
        return 0;
    }

    Header *header = (Header *)malloc(sizeof(Header));
    if (read(file, header->magic, 2) == 2) // read the magic number
    {
        header->magic[2] = '\0';
        if (header->magic[0] != 'T' && header->magic[1] != 'g') // check if the magic number is valid
        {
            if (print == 1)
                printf("ERROR\n wrong magic\n");
            free(header);
            close(file);
            return 0;
        }
    }

    if (read(file, &header->size, 2) != 2) // read the size
    {
        free(header);
        close(file);
        return 0;
    }
    if (read(file, &header->version, 1) == 1) // read the version
    {
        if (header->version < MIN_VERSION || header->version > MAX_VERSION) // check if the version is out of the specified range
        {
            if (print == 1)
                printf("ERROR\n wrong version\n");
            free(header);
            close(file);  
            return 0;
        }
    }
    if (read(file, &header->nr_sections, 1) == 1) // raed the nr of sections
    {
        if (header->nr_sections > 11 || (header->nr_sections < 5 && header->nr_sections != 2)) // check if the nr of sections is valid
        {
            if (print == 1)
                printf("ERROR\n wrong sect_nr\n");
            free(header);
            close(file);
            return 0;
        }
    }

    SectionHeader *sections = (SectionHeader *)malloc(header->nr_sections * sizeof(SectionHeader)); // allocate memory for the array of section headers

    for (int i = 0; i < header->nr_sections; i++) // for each section header from the file, read its information
    {
        read(file, sections[i].name, 10); // read the section name
        sections[i].name[10] = '\0';
        read(file, &sections[i].type, 1); // read the section type
        read(file, &sections[i].offset, 4); // read the section offset
        read(file, &sections[i].size, 4); // read the section size

        if ((int)sections[i].type != 46 && (int)sections[i].type != 72 && (int)sections[i].type != 71 && (int)sections[i].type != 21 && (int)sections[i].type != 40) // check if the section type is valid
        {
            if (print == 1)
                printf("ERROR\n wrong sect_types");
            free(header);
            free(sections);
            close(file);
            return 0;
        }
    }

    if (print == 1) // if printing is enabled, print the header information and the section details
    {
        printf("SUCCESS\n version=%d\n nr_sections=%d\n", header->version, header->nr_sections);
        for (int i = 0; i < header->nr_sections; i++)
            printf("section%d: %s %d %d\n", i + 1, sections[i].name, sections[i].type, sections[i].size);
    }

    free(header);
    free(sections);
    close(file);
    return 1; // return 1 if the given file respects the SF file format
}

void listDirectory(char *path, char *filtering_options)
{
    DIR *dir = NULL;
    dir = opendir(path);
    struct dirent *dirEntry = NULL;

    if (dir == NULL)
    {
        perror("Error opening directory");
        return;
    }
    else
        printf("SUCCESS\n");

    while ((dirEntry = readdir(dir)) != NULL)
    {
        char fullPath[1024];
        snprintf(fullPath, sizeof(path) + sizeof(dirEntry->d_name) + 1, "%s/%s", path, dirEntry->d_name);

        struct stat statbuf;
        if (lstat(fullPath, &statbuf) != 0)
            continue;

        if (filtering_options != NULL)
        {
            char *name_starts_with = strstr(filtering_options, "name_starts_with=");
            char *permissions = strstr(filtering_options, "permissions=");

            if (name_starts_with != NULL)
            {
                char *name = name_starts_with + strlen("name_starts_with=");
                if (strncmp(dirEntry->d_name, name, strlen(name)) != 0)
                    continue;
            }
            if (permissions != NULL)
            {
                char *perm = permissions + strlen("permissions=");
                struct stat statbuf;
                if (lstat(fullPath, &statbuf) != 0)
                    continue;
                if (!checkPermissions(statbuf.st_mode, perm))
                    continue;
            }
        }
        if (strcmp(dirEntry->d_name, "..") != 0 && strcmp(dirEntry->d_name, ".") != 0)
            printf("%s/%s\n", path, dirEntry->d_name); 
    }
    closedir(dir);
}

void listRec(char *basePath, char *filtering_options)
{
    DIR *dir = NULL; // pointer to directory stream
    struct dirent *entry = NULL; // pointer to directory entry
    struct stat statbuf; // structure to hold file status

    dir = opendir(basePath); // open the directory specified  by basePath
    if (dir == NULL)
    {
        perror("Couldn't open the directory!");
        return;
    }

    while ((entry = readdir(dir)) != NULL) // iterate through each entry in the directory
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) // ignore the "." and ".." directories 
        {
            char fullPath[1024];
            snprintf(fullPath, sizeof(basePath) + sizeof(entry->d_name) + 1, "%s/%s", basePath, entry->d_name); // construct the full path of the current entry

            if (filtering_options != NULL)
            {
                char *name_starts_with = strstr(filtering_options, "name_starts_with=");
                char *permissions = strstr(filtering_options, "permissions="); // extract the name to filter by

                if (name_starts_with != NULL) // check if filtering by name option is specified
                {
                    char *name = name_starts_with + strlen("name_starts_with="); // extract the name we need to filter by
                    if (strncmp(entry->d_name, name, strlen(name)) != 0) // if the entry name does not start with the specified name, skip it
                        continue;
                }
                if (permissions != NULL)
                {
                    char *perm = permissions + strlen("permissions="); // extract the permission string to filter by 
                    struct stat statbuf;
                    if (lstat(fullPath, &statbuf) != 0) // if we can't get the file status, skip it
                        continue;
                    if (!checkPermissions(statbuf.st_mode, perm)) 
                        continue;
                }
            }
            if (lstat(fullPath, &statbuf) == 0)
            {
                printf("%s\n", fullPath); // print the full path of the current entry
                if (S_ISDIR(statbuf.st_mode)) // check if the current entry is a directory
                    listRec(fullPath, filtering_options); //and if it is, recursively list the contents of the directory
            }
        }
    }
    closedir(dir); // close the directory stream
}

int checkPermissions(mode_t mode, char *perm)
{
    // check user permissions
    if ((perm[0] == 'r' && !(mode & S_IRUSR)) || (perm[0] == 'w' && !(mode & S_IWUSR)) || (perm[0] == 'x' && !(mode & S_IXUSR)))
        return 0;
    if ((perm[1] == 'w' && !(mode & S_IWUSR)) || (perm[1] == 'r' && !(mode & S_IRUSR)) || (perm[1] == 'x' && !(mode & S_IXUSR)))
        return 0;
    if ((perm[2] == 'x' && !(mode & S_IXUSR)) || (perm[2] == 'w' && !(mode & S_IWUSR)) || (perm[2] == 'r' && !(mode & S_IRUSR)))
        return 0;

    // check group permissions
    if ((perm[3] == 'r' && !(mode & S_IRGRP)) || (perm[3] == 'w' && !(mode & S_IWGRP)) || (perm[3] == 'x' && !(mode & S_IXGRP)))
        return 0;
    if ((perm[4] == 'w' && !(mode & S_IWGRP)) || (perm[4] == 'r' && !(mode & S_IRGRP)) || (perm[4] == 'x' && !(mode & S_IXGRP)))
        return 0;
    if ((perm[5] == 'x' && !(mode & S_IXGRP)) || (perm[5] == 'w' && !(mode & S_IWGRP)) || (perm[5] == 'r' && !(mode & S_IRGRP)))
        return 0;

    // check other permissions
    if ((perm[6] == 'r' && !(mode & S_IROTH)) || (perm[6] == 'w' && !(mode & S_IWOTH)) || (perm[6] == 'x' && !(mode & S_IXOTH)))
        return 0;
    if ((perm[7] == 'w' && !(mode & S_IWOTH)) || (perm[7] == 'x' && !(mode & S_IXOTH)) || (perm[7] == 'r' && !(mode & S_IROTH)))
        return 0;
    if ((perm[8] == 'x' && !(mode & S_IXOTH)) || (perm[8] == 'w' && !(mode & S_IWOTH)) || (perm[8] == 'r' && !(mode & S_IROTH)))
        return 0;

    return 1;
}
