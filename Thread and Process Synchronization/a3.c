#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>

#define RESP_PIPE "RESP_PIPE_71131"
#define REQ_PIPE "REQ_PIPE_71131"
#define SHM_NAME "/YoWTKwuX"
#define SHM_SIZE 4530838

int main(int argc, char **argv)
{
    int fd_resp = -1, fd_req = -1;
    int shm_fd = -1;
    char *shm_addr = NULL;
    char *file_addr = NULL;
    size_t file_size = 0;

    //unlink(RESP_PIPE);

    if (mkfifo(RESP_PIPE, 0600) != 0)
    {
        printf("ERROR\n cannot create the response pipe\n");
        return -1;
    }

    fd_req = open(REQ_PIPE, O_RDONLY | O_NONBLOCK); // Open in non-blocking mode
    if (fd_req == -1)
    {
        printf("ERROR\n cannot open the request pipe\n");
        unlink(RESP_PIPE);
        return -1;
    }

    fd_resp = open(RESP_PIPE, O_WRONLY);
    if (fd_resp == -1)
    {
        printf("ERROR\n cannot open the response pipe\n");
        close(fd_req);
        unlink(RESP_PIPE);
        return -1;
    }

    if (write(fd_resp, "\x42\x45\x47\x49\x4e\x24", 6) == 6) // write "BEGIN" in the response pipe
        printf("SUCCESS\n");
    else
    {
        printf("ERROR\n cannot write to the response pipe\n");
        close(fd_resp);
        close(fd_req);
        unlink(RESP_PIPE);
        return -1;
    }

    while (1)
    {
        char request[250];
        int i = 0;
        // int bytesRead = read(fd_req, request, 249);

        // if (bytesRead < 0)
        // {
        //     // No data available, sleep for a while and retry
        //     usleep(100000); // Sleep for 100ms
        //     continue;
        // }
        // else if (bytesRead == 0)
        // {
        //     // EOF reached
        //     break;
        // }

        // request[bytesRead] = '\0';
        //printf("%s \n", request);

        do{
            read(fd_req, &request[i], 1);
            i++;
        } while (request[i - 1] != '$');
        request[i] = '\0';

        if (strncmp(request, "EXIT", 4) == 0)
        {
            break;
        }
        else if (strncmp(request, "ECHO", 4) == 0)
        {
            if (write(fd_resp, "ECHO$", 5) == -1 ||
                write(fd_resp, "VARIANT$", 8) == -1 ||
                write(fd_resp, "\xdb\x15\x01\x00", 4) == -1) // 71131 in hex
            {
                printf("ERROR\n cannot write to the response pipe\n");
                break;
            }
        }
        else if (strncmp(request, "CREATE_SHM", 10) == 0)
        {
            unsigned int size;
            //sscanf(request + 11, "%x", &size);
            // int i = 11, j = 0;
            // char hexStr[200];
            // while(request[i] != '\0')
            // {
            //     sprintf(&hexStr[j], "%02x", request[i]);
            //     j += 2;
            //     i++;
            // }
            // hexStr[j] = '\0';
            // sscanf(hexStr, "%x", &size);
            //printf("%x\n", size);
            read(fd_req, &size, sizeof(unsigned int));
            shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0664);
            if(shm_fd < 0)
            {
                write(fd_resp, "CREATE_SHM$", 11);
                write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
            }
            else {
                ftruncate(shm_fd, size);
                shm_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if(shm_addr == MAP_FAILED)
                {
                    write(fd_resp, "CREATE_SHM$", 11);
                    write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
                }
                else {
                    write(fd_resp, "CREATE_SHM$", 11);
                    write(fd_resp, "\x53\x55\x43\x43\x45\x53\x53\x24", 8);
                }
            }
            close(shm_fd);
        }
        else if (strncmp(request, "WRITE_TO_SHM", 12) == 0)
        {
            unsigned int offset = 0, value = 0;

            // char *token = strtok(request + 12, "$");
            // offset = atoi(token);
            // token = strtok(NULL, "\0");
            // value = atoi(token);
            // printf("%d %d\n", offset, value);

            //sscanf(request + 13, "%02x %02x", &offset, &value);

            read(fd_req, &offset, sizeof(unsigned int));
            read(fd_req, &value, sizeof(unsigned int));
           
            if (shm_addr && offset >= 0 && offset <= 4530838 && (offset + sizeof(value) <= SHM_SIZE))
            {
                memcpy(shm_addr + offset, &value, sizeof(value));
                write(fd_resp, "WRITE_TO_SHM$", 13); 
                write(fd_resp, "\x53\x55\x43\x43\x45\x53\x53\x24", 8);
            }
            else
            {
                write(fd_resp, "WRITE_TO_SHM$", 13); 
                write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
            }
        }
        else if (strncmp(request, "MAP_FILE", 8) == 0)
        {
            char fileN[250];
            // sscanf(request + 9, "%s", fileN);
            // int i = strlen(fileN);
            // char file_name[i];
            // strncpy(file_name, fileN, i - 1);
            // file_name[i - 1] = '\0';
            //printf("%s\n", file_name);

            int bytesRead = read(fd_req, fileN, sizeof(fileN));
            fileN[bytesRead - 1] = '\0';
            
            int fd = open(fileN, O_RDONLY);
            if (fd == -1)
            {
                write(fd_resp, "MAP_FILE$", 9); 
                write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
            }
            else
            {
                struct stat st;
                fstat(fd, &st);
                file_size = st.st_size;
                file_addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
                if (file_addr == MAP_FAILED)
                {
                    write(fd_resp, "MAP_FILE$", 9); 
                    write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
                }
                else
                {
                    write(fd_resp, "MAP_FILE$", 9); 
                    write(fd_resp, "\x53\x55\x43\x43\x45\x53\x53\x24", 8);
                }
            }
        }
        else if (strncmp(request, "READ_FROM_FILE_OFFSET", 21) == 0)
        {
            unsigned int offset, no_of_bytes;
            //sscanf(request + 22, "%02x %02x", &offset, &no_of_bytes);
            read(fd_req, &offset, sizeof(unsigned int));
            read(fd_req, &no_of_bytes, sizeof(unsigned int));
            if (file_addr && offset + no_of_bytes <= file_size)
            {
                memcpy(shm_addr, file_addr + offset, no_of_bytes);
                write(fd_resp, "READ_FROM_FILE_OFFSET$", 22); 
                write(fd_resp, "\x53\x55\x43\x43\x45\x53\x53\x24", 8);
            }
            else
            {
                write(fd_resp, "READ_FROM_FILE_OFFSET$", 22); 
                write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
            }
        }
        else if (strncmp(request, "READ_FROM_FILE_SECTION", 22) == 0)
        {
            unsigned int section_no, offset, no_of_bytes;
            //sscanf(request + 23, "%u %u %u", &section_no, &offset, &no_of_bytes);
            read(fd_req, &section_no, sizeof(unsigned int));
            read(fd_req, &offset, sizeof(unsigned int));
            read(fd_req, &no_of_bytes, sizeof(unsigned int));
            
            if (file_addr)
            {
                unsigned int section_size = 0, section_offset = 0, nr_sections = 0;
                memcpy(&nr_sections, file_addr + 5, 1);
                if(section_no > nr_sections)
                {
                    write(fd_resp, "READ_FROM_FILE_SECTION$", 23); 
                    write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
                } else {
                    memcpy(&section_offset, file_addr + 6 + 19 * (section_no - 1) + 11, 4);
                    memcpy(&section_size, file_addr + 6 + 19 * (section_no- 1) + 15, 4);
                    if(offset + no_of_bytes > section_size)
                    {
                        write(fd_resp, "READ_FROM_FILE_SECTION$", 23); 
                        write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
                    } else {
                        memcpy(shm_addr, file_addr + section_offset + offset, no_of_bytes);
                        write(fd_resp, "READ_FROM_FILE_SECTION$", 23); 
                        write(fd_resp, "\x53\x55\x43\x43\x45\x53\x53\x24", 8);
                    }
                }
            }
            else
            {
                write(fd_resp, "READ_FROM_FILE_SECTION$", 23); 
                write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
            }
        }
        else if (strncmp(request, "READ_FROM_LOGICAL_SPACE_OFFSET", 30) == 0)
        {
            unsigned int logical_offset, no_of_bytes;
            //sscanf(request + 31, "%u %u", &logical_offset, &no_of_bytes);
            read(fd_req, &logical_offset, sizeof(unsigned int));
            read(fd_req, &no_of_bytes, sizeof(unsigned int));
            if (file_addr)
            {
                unsigned int actual_offset = 0;
                unsigned int num_sections = *(file_addr + 5);

                for(int i = 0; i < num_sections; i++)
                {
                    unsigned int section_size = *(file_addr + 6 + 19 * i + 15);
                    if(logical_offset >= actual_offset && logical_offset < actual_offset + section_size) // check if the logical offset is within the current section
                    {
                        actual_offset += logical_offset - actual_offset;
                        break;
                    }
                    actual_offset += section_size; // move to the next section
                }

                if(actual_offset + no_of_bytes <= file_size) // check if the actual offset is valid
                {
                    memcpy(shm_addr, file_addr + actual_offset, no_of_bytes);
                    write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31); 
                    write(fd_resp, "\x53\x55\x43\x43\x45\x53\x53\x24", 8);
                } else
                {
                    write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31); 
                    write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
                }
            }
            else
            {
                write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET$", 31); 
                write(fd_resp, "\x45\x52\x52\x4f\x52\x24", 6);
            }
        }
    }

    // Cleanup
    if (shm_addr)
    {
        munmap(shm_addr, SHM_SIZE);
        shm_unlink(SHM_NAME);
    }
    if (file_addr)
    {
        munmap(file_addr, file_size);
    }
    // close the pipes
    close(fd_resp);
    close(fd_req);
    unlink(RESP_PIPE);

    return 0;
}