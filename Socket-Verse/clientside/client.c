#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <fcntl.h>

//constatns
#define PORT 8080
#define MIRROR_PORT 7001
#define BUFSIZE 1024
#define MAX_LENGTH_OF_COMMAND 10000

// flags
bool needToUnzip = false;
bool returns_tar_file = false;

// function to send message to server
void send_control_message(int skt_fd, char *msg)
{
  write(skt_fd, msg, strlen(msg));
}


// this function responsible for receiveing file
void receive_file(int file_fd, int socket)
{
  //check error
  if (file_fd < 0)
  {
    perror("Error creating file\n");
    return;
  }

  char buffer[BUFSIZE];
  int bytesReceived;

  // read from the socket fd
  while ((bytesReceived = read(socket, buffer, BUFSIZE)) > 0)
  {
    // printf("%d received\n", bytesReceived);
    // write to the new tar file
    write(file_fd, buffer, bytesReceived);

    // if theno. of bytes received are less the buff size exit
    // this means there are no more character in socket
    if (bytesReceived < BUFSIZE)
      break;
  }

  printf("Done\n");
}

// function receives message
void receive_message(int socket, char *buffer)
{

  ssize_t bytesReceived = read(socket, buffer, BUFSIZE);
  if (bytesReceived > 0)
  {
    printf("Message from server: %s\n", buffer);
  }
}

// function reveives control message
void receive_control_message(int socket, char *buffer)
{
  // FIL, MIR, ERR, CTM, CTS, QIT
  ssize_t bytesReceived = read(socket, buffer, 3);
  // if (bytesReceived > 0)
  // {
  //   printf("Control from server: %s\n", buffer);
  // }
}

// unzip the file
void unzipFile(char *fileName)
{
  char command_buf[BUFSIZE];
  int status;

  // unzip file using tar command -x to unzip
  sprintf(command_buf, "tar -xzf \"%s\" 2>/dev/null",
          fileName);

  // call system command to execute the the command
  status = system(command_buf);

  if (status != 0)
  {
    printf("Some error Occured while unzipping the file..\n");
  }
  else
  {
    printf("File Unzipped Successfully.\n");
  }
}

// this function checks if the parameter is integer and is greater than 0
long is_valid_digits_range(char *val)
{
  char *helperPtr;
  long number;

  // Use strtol to check if val is valid integer
  number = strtol(val, &helperPtr, 10);

  if (helperPtr == val || number < 0)
  {
    // not a integer grater than 0
    printf("Size has to be positive integer %s\n", val);
    return -1;
  }
  else
  {
    // valid integer
    return number;
  }
}

// for getdirf command check if the dates are valid
bool is_valid_dates(const char *sdate1, const char *sdate2)
{

  // year, month , date
  int date1[3];
  int date2[3];
  int year, month, day;

  // Check if the sdate1 matches the date format "YYYY-MM-DD"
  if (sscanf(sdate1, "%d-%d-%d", &year, &month, &day) == 3)
  {
    // range possibility
    if ((year >= 1000 && year <= 9999) &&
        (month >= 1 && month <= 12) &&
        (day >= 1 && day <= 31))
    {
      date1[0] = year;
      date1[1] = month;
      date1[2] = day;
    }
    else
    {
      printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate1);
      return false;
    }
  }
  else
  {
    printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate2);
    return false;
  }

  // Check if the sdate2 matches the date format "YYYY-MM-DD"
  if (sscanf(sdate2, "%d-%d-%d", &year, &month, &day) == 3)
  {
    // range possibility
    if ((year >= 1000 && year <= 9999) &&
        (month >= 1 && month <= 12) &&
        (day >= 1 && day <= 31))
    {
      date2[0] = year;
      date2[1] = month;
      date2[2] = day;
    }
    else
    {
      printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate2);
      return false;
    }
  }
  else
  {
    printf("Invalid Date format(YYYY-MM-DD) %s\n", sdate2);
    return false;
  }
  // printf("time to validate\n");
  // check if date1 <= date2
  // Compare the year
  if (date1[0] < date2[0])
    return true;
  else if (date1[0] > date2[0])
  {
    printf("date1 is greater than date2\n");
    return false;
  }

  // Compare the month if year is equal
  if (date1[1] < date2[1])
    return true;
  if (date1[1] > date2[1])
  {
    printf("date1 is greater than date2\n");
    return false;
  }

  // Compare the day
  if (date1[2] <= date2[2])
    return true;

  // date1 is greater than date2
  printf("date1 is greater than date2\n");
  return false;
}

// validate the syntax of commands
bool validate_the_command(char *command)
{

  // tokenize the command
  char command_with_args[10][PATH_MAX];
  int size = 0;

  // used for tokenizing
  char *tempPtr;

  // tokkenize on space using strtok_r
  char *token = strtok_r(command, " ", &tempPtr);

  // loop through all the tokens
  while (token != NULL)
  {
    // copy string to array
    strcpy(command_with_args[size++], token);

    // tokenize
    token = strtok_r(NULL, " ", &tempPtr);
  }

  // command_with_args[0] contains the name of command

  if (strcmp(command_with_args[0], "fgets") == 0)
  {

    // where the command is fgets
    // fgets f1 f2 f3 f4 so max no. of tokens is 5
    if (size < 2 || size > 5)
    {
      printf("Wrong input: Number of files allowed is between 1 to 4\n");
      return false;
    }
    returns_tar_file = true;
    return true;
  }
  else if (strcmp(command_with_args[0], "tarfgetz") == 0)
  {

    // where the command is tarfgetz
    // tarfgetz size1 size2 <-u> so max no. of tokens is 4
    if (size < 3){
      printf("Wrong input: Allowed input format is tarfgetz size1 size2 <-u>\n");
      return false;
    }
    int n1 = is_valid_digits_range(command_with_args[1]);
    int n2 = is_valid_digits_range(command_with_args[2]);
    // if it has -u option
    if (strcmp(command_with_args[size - 1], "-u") == 0 && size == 4 && n1 != -1 && n2 != -1 && n1 <= n2)
    {
      // this is valid
      needToUnzip = true;
    }
    else if (strcmp(command_with_args[size - 1], "-u") != 0 && size == 3 && n1 != -1 && n2 != -1 && n1 <= n2)
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input format is tarfgetz size1 size2 <-u>\n");
      return false;
    }
    returns_tar_file = true;
    return true;
  }
  else if (strcmp(command_with_args[0], "filesrch") == 0)
  {
    // where the command is filesrch
    // filesrch filename so max no. of tokens is 2
    if (size != 2)
    {
      printf("Wrong input: Allowed format is  filesrch filename  \n");
      return false;
    }

    return true;
  }
  else if (strcmp(command_with_args[0], "targzf") == 0)
  {

    // where the command is targzf
    // targzf <extension list> <-u> up to 4 different file types  max no. of token is 6

    // if it has -u option
    if (strcmp(command_with_args[size - 1], "-u") == 0 && size > 2 && size <= 6)
    {
      // this is valid
      needToUnzip = true;
    }
    else if (strcmp(command_with_args[size - 1], "-u") != 0 && size >= 2 && size < 6)
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input targzf <extension list> <-u>\n");
      return false;
    }

    returns_tar_file = true;
    return true;
  }
  else if (strcmp(command_with_args[0], "getdirf") == 0)
  {

    // where the command is getdirf
    // getdirf date1 date2 <-u> max no. of tokens is 4

    // if it has -u option check size of command and check if date1< date2 and are valid dates
    if (strcmp(command_with_args[size - 1], "-u") == 0 && size == 4 && is_valid_dates(command_with_args[1], command_with_args[2]))
    {
      // this is valid
      needToUnzip = true;
    }
    // if it doesn't have -u option check size of command and check if date1< date2 and are valid dates
    else if (strcmp(command_with_args[size - 1], "-u") != 0 && size == 3 && is_valid_dates(command_with_args[1], command_with_args[2]))
    {
      // no -u option
      // this is valid
    }
    else
    {
      printf("Wrong input: Allowed input format getdirf date1 date2 <-u>\n");
      return false;
    }

    returns_tar_file = true;
    return true;
  }
  else if (strcmp(command_with_args[0], "quit") == 0)
  {
    // quit the client
    return true;
  }
  else
  {
    printf("Sorry, Wrong input.\n");
    return false;
  }

  return false;
}

int main(int argc, char const *argv[])
{

  int skt_fd;
  struct sockaddr_in serv_addr, mirror_addr;

  char buff[BUFSIZE] = {'\0'};
  char command[BUFSIZE] = {'\0'};

  // create a socket and handle error
  if ((skt_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error \n");
    return -1;
  }

  // set server address to NULL
  memset(&serv_addr, '\0', sizeof(serv_addr));

  // set port number and other parameters
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
  {
    printf("\n the address is invalid / Address not supported \n");
    return -1;
  }

  // create a connection to server from client side
  if (connect(skt_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection has Failed \n");
    return -1;
  }

  // clear the buffer
  char buffer[BUFSIZE] = {0};
  char resp[BUFSIZE] = {0};
  // check the control message
  receive_control_message(skt_fd, buffer);

  // if the control message is MIR redirect client to mirror
  if (strcmp(buffer, "MIR") == 0)
  {
    // closing the current server connection
    close(skt_fd);

    // Create new socket for the mirror server
    skt_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (skt_fd == -1)
    {
      perror("socket");
      exit(EXIT_FAILURE);
    }

    memset(&mirror_addr, '\0', sizeof(mirror_addr));
    mirror_addr.sin_family = AF_INET;
    mirror_addr.sin_port = htons(MIRROR_PORT);
    mirror_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the mirror server
    if (connect(skt_fd, (struct sockaddr *)&mirror_addr, sizeof(mirror_addr)) == -1)
    {
      perror("connect");
      exit(EXIT_FAILURE);
    }

    // check the control message exepectting CTM now
    receive_control_message(skt_fd, buffer);

  }

  // if control message iis CTM
  if (strcmp(buffer, "CTM") == 0)
  {
    printf("Connected to Mirror.\n");
  }
  // if control message is CTS
  else if (strcmp(buffer, "CTS") == 0)
  {
    printf("Connected to server.\n");
  }

  char inputByUser[MAX_LENGTH_OF_COMMAND];
  char copyOfUserInput[MAX_LENGTH_OF_COMMAND];

  while (1)
  {

    printf("\nEnter a command\n-filesrch <filename>\n-fgets <file1> <file2> <file3> <file4>\n-tarfgetz <size1> <size2> <-u>\n-getdirf <date1> <date2> <-u>\n-targzf <ext1> <ext2> <ext3> <ext4> <-u>\n-quit\n");

    // flags
    needToUnzip = false;
    returns_tar_file = false;

    // getting user input, from standard input
    fgets(inputByUser, MAX_LENGTH_OF_COMMAND, stdin);

    // replaces the \n with NULL character in user input
    inputByUser[strcspn(inputByUser, "\n")] = '\0';
    
    // creating a copy of inputByUser
    strncpy(copyOfUserInput, inputByUser, MAX_LENGTH_OF_COMMAND);

    // validate the command syntax
    bool commandValidation = validate_the_command(inputByUser);

    // clear buffer and continue if command validation is false
    if (!commandValidation)
      continue;

    /// else the commandValidation is true good to go
    

    memset(resp, 0, sizeof(resp));
    memset(buffer, 0, sizeof(buffer));

    fflush(stdout);
    // Send input by user to server
    send_control_message(skt_fd, copyOfUserInput);

    // check the control message
    receive_control_message(skt_fd, buffer);

    // if thte control message is FIL the server will be sending a file
    if (strcmp(buffer, "FIL") == 0)
    {

      int file_fd;

      // check if the command requires tar file as response
      if (returns_tar_file)
      {
        // create a new temp.tar.gz
        file_fd = open("temp.tar.gz", O_WRONLY | O_CREAT | O_TRUNC, 0777);

        printf("Receiving ....\n");

        // receive file from server
        receive_file(file_fd, skt_fd);

        printf("File received\n");

        // close file desc
        close(file_fd);

        // if there is -u option
        // unzip the file
        if (needToUnzip)
        {
          printf("Unzipping file...\n");
          // call the function
          unzipFile("temp.tar.gz");
        }
      }
      else
      {
        // handle error
        printf("Some error occured, Server is trying to send a file but this command doesnot accepts file as a response.\n");
      }
    }
    // if there is a control message of ERR
    else if (strcmp(buffer, "ERR") == 0)
    {
      // recive the message from server in resp
      receive_message(skt_fd, resp);
    }

    // if the server is returning message in case of  FILESRCH
    else if (strcmp(buffer, "MSG") == 0)
    {
      // recceive message
      receive_message(skt_fd, resp);
    }

    // if there is quit signal dfrom server
    else if (strcmp(buffer, "QIT") == 0)
    {
      // close socket fd
      close(skt_fd);
      printf("Connection is closed.\n");
      // exit this process.
      exit(EXIT_SUCCESS);
    }
  }

  // just for  precaution purpose might not get called.
  close(skt_fd);
  printf("Connection is closed.\n");

  exit(EXIT_SUCCESS);
}
