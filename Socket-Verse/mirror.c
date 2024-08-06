#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>

//cponstants
#define PORT 7001
#define _XOPEN_SOURCE 500
#define BUFSIZE 1024

// root dir
char *rootDirectory = "$HOME";

struct
{
  char *file_name;
  int file_size;
  char *file_created_date;
} File_info;

int day, year, hour, min, sec;
char month[4];

// function to send message to client
void sendControlMessage(int skt_fd, char *msg)
{
  write(skt_fd, msg, strlen(msg));
}

// function to send message to client
void sendMessage(int skt_fd, char *msg)
{
  write(skt_fd, msg, strlen(msg));
}

// function to send tar file to the client
bool sendFileToClient(char *file, int socket_fd)
{

  // open file in read mode
  int fileTosend = open(file, O_RDONLY);
  // handle error
  if (fileTosend < 0)
  {
    perror("Error in opening the temp.tar.gz file");
    return false;
  }

  char buffer[BUFSIZE];
  ssize_t bytesRead;

  // read file and write into socket fd
  while ((bytesRead = read(fileTosend, buffer, sizeof(buffer))) > 0)
  {

    // printf("bytes read: %d\n", bytesRead);

    if (write(socket_fd, buffer, bytesRead) == -1)
      perror("Sending file failed");

    // check if bytes read is less than buffer size
    if (bytesRead < sizeof(buffer))
      break;
  }

  // close file
  close(fileTosend);
  return true;
}

/*
  This function is responsible for handling filesrch command
*/
void file_search(char *base_path, char *filename)
{
  char command_buf[BUFSIZE];

  // Searching for file in the root directory
  sprintf(command_buf, "find %s -type f -wholename $(find %s -type f -name %s | awk -F/ '{ print NF-1, $0 }' | sort -n | awk '{$1=\"\"; print $0}'|head -1) -printf \"%%s,%%Tc\n\" 2>/dev/null", base_path, base_path, filename);

  FILE *fp = popen(command_buf, "r");
  char line[BUFSIZE];

  // if File found in root directory
  if (fgets(line, BUFSIZE, fp) != NULL)
  {
    File_info.file_name = strdup(filename);
    File_info.file_size = atoi(strtok(line, ","));

    char *date = strtok(NULL, ",");
    if (sscanf(date, "%*s %s %d %d:%d:%d %d", month, &day, &hour, &min, &sec, &year) == 6)
      sprintf(date, "%s %d, %d %d:%d:%d\n", month, day, year, hour, min, sec);
    File_info.file_created_date = strdup(date);
  }
  // if File not found
  else
    File_info.file_size = -1;

  pclose(fp);
}

/*
  This function is responsible for handling fgets command
*/
int get_files(char *base_path, char *file1, char *file2, char *file3, char *file4)
{

  int status = 0;
  char command_buf[BUFSIZE];

  /*
    create the command
    -type f gives files only, -iname matches name with case insensitive
    -print0 also handles white spaces efficiently
    xargs split file name on null character and pass it to tar
    -c creaate, -z creta a gzip, -f file name for tar
    stderror(2) is redirected to dev/null
  */

  sprintf(command_buf, "find %s -type f \\( -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" -o -iname \"%s\" \\) -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null",
          base_path, file1, file2, file3, file4);

  // call system command to execute the the command
  status = system(command_buf);

  return status == 0 ? true : false;
}
/*
  This function is responsible for handling tarfgetz command
*/
bool get_files_matching_size(char *base_path, int size1, int size2)
{
  char command_buf[BUFSIZE];
  sprintf(command_buf, "find %s -type f -size +%dc -a -size -%dc -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null",
          base_path, size1, size2);
  int status = system(command_buf);


  return status == 0 ? true : false;
}

/*
  This function is responsible for handling getdirf command
*/
bool get_files_matching_date(char *base_path, char *date1, char *date2)
{

  char command_buf[BUFSIZE];
  sprintf(command_buf, "find %s -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null",
          base_path, date1, date2);

  int status = system(command_buf);

  return status == 0 ? true : false;
}

bool get_files_matching_ext(char *base_path, char *ext1, char *ext2, char *ext3, char *ext4)
{
  char command_buf[BUFSIZE];
  sprintf(command_buf, "find %s -type f \\( ", base_path);
  if (ext1 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext1);
  if (ext2 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext2);
  if (ext3 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext3);
  if (ext4 != NULL)
    sprintf(command_buf + strlen(command_buf), "-iname \"*.%s\" -o ", ext4);

  sprintf(command_buf + strlen(command_buf), "-false \\) -print0 | xargs -0 tar -czf temp.tar.gz 2>/dev/null");

  int status = system(command_buf);

  return status == 0 ? true : false;
}

/*
  This is the main function which handles the command from client

*/

void processclient(int skt_fd)
{
  char cmd[BUFSIZE] = {'\0'};
  char response[BUFSIZE * 2] = {'\0'};

  // running and infinite loop to read the commands
  // until the connection is closed or quit commmand is received
  while (true)
  {

    // Clear all the arrays
    memset(cmd, 0, sizeof(cmd));
    memset(response, 0, sizeof(response));

    // read the input from the socket file descriptor
    int sizeOfInput = read(skt_fd, cmd, sizeof(cmd));
    // add null character at the end of the command
    cmd[sizeOfInput] = '\0';

    // print thte command 
    if(sizeOfInput > 0)
      printf("Executing: %s\n", cmd);

    // Parse command by tokenizing
    char *token = strtok(cmd, " ");

    // handle commands
    if (strcmp(token, "fgets") == 0)
    {
      // execcution of fgets command

      // get the first file
      char *file1 = strtok(NULL, " ");

      // if there is no file name given
      if (file1 == NULL)
        sprintf(response, "The syntax is Invalid. Please try again.\n");
      else
      {
        // as minimum requirement is 1 file which is satisfied
        // get other at max 3 files

        char *file2 = strtok(NULL, " ");
        char *file3 = strtok(NULL, " ");
        char *file4 = strtok(NULL, " ");

        // call the function to get the files
        int status = get_files(rootDirectory, file1, file2, file3, file4);

        // check if the files were found
        if (status)
        {
          // if files received
          // Sending control message
          sendControlMessage(skt_fd, "FIL");

          // send file
          bool isSent = sendFileToClient("temp.tar.gz", skt_fd);

        }
        else
        {
          // No files found
          sendControlMessage(skt_fd, "ERR");
          sendMessage(skt_fd, "Error: No Files Found\n");
        }
      }
    }
    else if (strcmp(token, "filesrch") == 0)
    {
      char *filename = strtok(NULL, " ");
      if (filename == NULL)
        sprintf(response, "The syntax is Invalid. Please try again.\n");
      else
      {
        // fsearch logic
        File_info.file_size = 0;
        // serach file by file name in root ($HOME) directory
        file_search(rootDirectory, filename);

        sprintf(response, "Name: %s\t\tSize: %d bytes\t\tCreated Date: %s\n", File_info.file_name, File_info.file_size, File_info.file_created_date);

        if (File_info.file_size == -1)
        {
          sendControlMessage(skt_fd, "ERR");
          sendMessage(skt_fd, "Error: File not found.\n");
        }
        else
        {
          sendControlMessage(skt_fd, "MSG");
          sendMessage(skt_fd, response);
        }
      }
    }
    else if (strcmp(token, "tarfgetz") == 0)
    {
      char *size1_str = strtok(NULL, " ");
      char *size2_str = strtok(NULL, " ");

      int size1 = atoi(size1_str);
      int size2 = atoi(size2_str);

      // get files matching the size range
      bool status = get_files_matching_size(rootDirectory, size1, size2);

      // check if the files were found
      if (status)
      {
        // if files received
        // Sending control message
        sendControlMessage(skt_fd, "FIL");

        // send file
        bool isSent = sendFileToClient("temp.tar.gz", skt_fd);

      }
      else
      {
        sendControlMessage(skt_fd, "ERR");
        sendMessage(skt_fd, "Error: No files found.\n");
      }
    }
    else if (strcmp(token, "getdirf") == 0)
    {
      char *date1_str = strtok(NULL, " ");
      char *date2_str = strtok(NULL, " ");


      // get files matching date range
      bool status = get_files_matching_date(rootDirectory, date1_str, date2_str);

      // check if any files were found
      if (status)
      {
        // if files received
        // Sending control message
        sendControlMessage(skt_fd, "FIL");

        // send file
        bool isSent = sendFileToClient("temp.tar.gz", skt_fd);

      }
      else
      {
        sendControlMessage(skt_fd, "ERR");
        sendMessage(skt_fd, "Error: No files found.\n");
      }
    }
    else if (strcmp(token, "targzf") == 0)
    {
      char *ext1 = strtok(NULL, " ");
      char *ext2 = strtok(NULL, " ");
      char *ext3 = strtok(NULL, " ");
      char *ext4 = strtok(NULL, " ");


      // check if any of the specified files are present
      bool status = get_files_matching_ext(rootDirectory, ext1, ext2, ext3, ext4);

      // check if any files were found
      if (status)
      {
        // if files received
        // Sending control message
        sendControlMessage(skt_fd, "FIL");

        // send file
        bool isSent = sendFileToClient("temp.tar.gz", skt_fd);

      }
      else
      {
        sendControlMessage(skt_fd, "ERR");
        sendMessage(skt_fd, "Error: No files found.\n");
      }
    }
    else if (strcmp(token, "quit") == 0)
    {
      // send quit control signal to client
      sendControlMessage(skt_fd, "QIT");

      printf("Closing client connection.\n");

      // sleep for a second before quit
      sleep(1);

      // close socket
      close(skt_fd);

      // exit the child fork
      exit(EXIT_SUCCESS);
    }
  }

  close(skt_fd);
  exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[])
{
  int srv_fd, new_skt;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // Create socket file descriptor
  if ((srv_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    //error
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Attach socket to the port 8080
  if (setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
  {
    //handle error
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(srv_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(srv_fd, 3) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // waiting for client
	printf("Waiting for client...\n");

  while (true)
  {
    if ((new_skt = accept(srv_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      //error
      perror("Error in accept");
      exit(EXIT_FAILURE);
    }
    
    // sedn control message to client "CTM(Connected to mirror)"
		sendControlMessage(new_skt,"CTM");
    printf("The New client is connected.\n");

    pid_t pid = fork();
    if (pid == 0)
    {
      // Child process
      close(srv_fd);
      processclient(new_skt);
    }
    else if (pid == -1)
    {
      // error
      perror("fork");
      exit(EXIT_FAILURE);
    }
    else
    {
      // Parent process
      close(new_skt);
      while (waitpid(-1, NULL, WNOHANG) > 0)
        ; // Clean up zombie processes
    }
  }

  exit(EXIT_SUCCESS);
}
