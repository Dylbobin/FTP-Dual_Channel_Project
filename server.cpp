#include "parse_lib.h"
#include <fstream>
#include <iostream>
#include <limits.h> /* For limits */ /* For tcp_recv, tcp_recv_size, tcp_send_size, and tcp_send */
#include <netinet/in.h> /* Contains constants and structures needed for internet domain addresses. */
#include <ostream>
#include <stdio.h>      /* Contains common I/O functions */
#include <stdlib.h>     /* For atoi() and exit() */
#include <string.h>     /* For memset() */
#include <sys/socket.h> /* Includes definitions of structures needed for sockets */
#include <sys/types.h> /* Contains definitions of data types used in system calls */
#include <sys/wait.h>
#include <time.h>   /* For retrieving the time */
#include <unistd.h> /* Contains standard unix functions */
using namespace std;

#define MAX_MSG 100
#define MAX_FILE_CHUNK_SIZE 100
#define TIME_BUFF_SIZE 100
#define MAX_FILE_NAME_SIZE 100
#define MAX_LENGTH 100

void establish_transfer_channel(int &transfer_port, int &transfer_con, sockaddr_in &transferAddr, int connfd) {
  char transfer_port_number_recv[MAX_LENGTH];
  recv(connfd, transfer_port_number_recv, 6, 0);
  sleep(4);
  cout << "Transfer_port_number = ";
  for (int i = 0; i < 6; i++) {
    cout << transfer_port_number_recv[i];
  }
  cout << endl;
  transfer_port = atoi((char*)transfer_port_number_recv);
  cout << "Transfer port number is " << transfer_port << endl;

  // Open the channel for the server to listen on
  transfer_con = socket(AF_INET, SOCK_STREAM, 0);
  if (transfer_con < 0) {
    cout << "ERROR from Server: Unable to open socket\n";
    exit(-1);
  }

  // Set the structure to all zeros
  memset(&transferAddr, 0, sizeof(transferAddr));

  // Set the transfer family
  transferAddr.sin_family = AF_INET;

  // Convert the port number to network representation
  transferAddr.sin_port = htons(transfer_port);

  // Connect to the File Transfer Channel
  int success_connect =
      connect(transfer_con, (sockaddr *)&transferAddr, sizeof(sockaddr));
  cout << "Transfer_con inside the establish connection function = " << ntohs(transfer_con) << endl;
}

int main(int argc, char **argv) {
  // The buffer to receive
  char recv_buff[MAX_FILE_CHUNK_SIZE];
  char org_file_name[MAX_FILE_NAME_SIZE];
  int file_size = 0;

  int port = -1;
  int transfer_port = -1;

  // Sockets for server to lisen and client conection
  int listenfd = -1;
  int connfd = -1;
  int transfer_con = -1;

  // Initiate the server/client address
  sockaddr_in serverAddr, clientAddr, transferAddr;

  // Size of the client's address
  socklen_t lient_length = sizeof(clientAddr);
  socklen_t transfer_length = sizeof(transferAddr);

  // If port not provided, exit
  if (argc < 2) {
    cout << "Please provide a port number\n";
    return -1;
  }

  // Assign port number
  port = atoi(argv[1]);

  // Make sure that the port is within a valid range
  if (port < 0 || port > 65535) {
    cout << "Invalid port number\n";
    return -1;
  }

  // Create a socket that uses (AF_INET), SOCK_STREAM)
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    cout << "ERROR: Unable to create a socket\n";
    return -1;
  }

  // Set address to all 0's
  memset(&serverAddr, 0, sizeof(serverAddr));

  // Convert the port number to network
  serverAddr.sin_port = htons(port);

  // Server to IPv4
  serverAddr.sin_family = AF_INET;

  /* Allow retreival of packets without having to know your IP address
   and retrieve packets from all network interfaces */
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Bind the adress to the soocket
  if (bind(listenfd, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    cout << "ERROR: Unable to bind\n";
    return -1;
  }

  // Listen to up to 50 connections
  if (listen(listenfd, 50) < 0) {
    cout << "ERROR: listen\n";
    return -1;
  }
  bool running = true;
  while (running) {
    cout << "Waiting for somebody to connect on port " << port << endl;

    // Allow a client to connect and have their details stored
    if ((connfd = accept(listenfd, (sockaddr *)&clientAddr, &lient_length)) <
        0) {
      cout << "ERROR: Unable to accept\n";
      exit(-1);
    }

    cout << "Connected!\n";

    char msg_received[MAX_LENGTH];
    int num_recv = 0;
    int total_bytes_recv = 0;

    // Receive hello from the client
    recv(connfd, msg_received, MAX_LENGTH, 0);

    cout << "Input From Client: " << (string)msg_received << endl;

    string command_name = "temp";

    while (command_name != "quit") {
      // Value to store the command
      char cmd_input[MAX_LENGTH];

      num_recv = 0;
      while (num_recv <= 0) {

        char wait[MAX_LENGTH];
        recv(connfd, wait, MAX_LENGTH, 0);
        num_recv = ((string)wait).length();
        if (num_recv > 0) {
          int i = 0;
          while (wait[i] != '\n') {
            cmd_input[i] = wait[i];
            i++;
          }
          cmd_input[i] = '\n';
        }
      }

      cout << "Received = " << (string)cmd_input << endl;

      string file_name;

      if (cmd_input[0] == 'p' || cmd_input[0] == 'g') {
        file_name = parse_filename(cmd_input);
        command_name = parse_operation(cmd_input);

      } else {
        command_name = cmd_input;
      }

      if ((string)command_name == "get") {
        cout << "Working Get..." << endl;
        establish_transfer_channel(transfer_port, transfer_con, transferAddr, connfd);

        cout << "  The name of the file is " << file_name << endl;
        cout << "  The chosen operation is " << command_name << endl;

        ifstream file;
        file.open(file_name.c_str());
        if (!file) {
          cout << "ERROR: SERVER unable to open file\n";
        }

        int total_bytes_sent = 0;
        while (!file.eof()) {
          string message;
          getline(file, message, '\n');

          message += '\n';
          cout << message;

          int num_bytes_sent = 0;
          char message_to_send[1000];
          int i = 0;
          for (; i < message.length(); i++) {
            message_to_send[i] = message[i];
          }

          if (!file.eof()) {
          send(transfer_con, message_to_send, message.length(), 0);
          sleep(2);
          num_bytes_sent = message.length();

          if (num_bytes_sent < 0) {
            cout << "ERROR from Server: unable to send ftp command\n";
          }
          total_bytes_sent += num_bytes_sent;
        }
        else {
          message_to_send[0] = '\n';
          send(transfer_con, message_to_send, 1, 0);
         }
        }

        file.close();

        cout << "Sent " << total_bytes_sent << " bytes of data\n";

        // Close the File Transfer channel
        close(transfer_con);
      } else if ((string)command_name == "put") {
        cout << "Working Put..." << endl;

        // Open the File Transfer channel for receiving the data
        establish_transfer_channel(transfer_port, transfer_con, transferAddr, connfd);

        ifstream open_file;
        string org_file_name;
        open_file.open(file_name);
        if (open_file) {
          int i = 0;

          while (i < file_name.length() - 1) {
            char character = file_name[i];
            if (character != '.') {
              org_file_name += character;
              i++;
            } else {
              org_file_name += "(1)";
              org_file_name += file_name[i];
              i++;
              org_file_name += file_name[i];
              i++;
              org_file_name += file_name[i];
              i++;
              org_file_name += file_name[i];
              break;
            }
          }
        } else {
          org_file_name = file_name;
        }

        ifstream glossary("glossary.txt");

        string line;
        bool notFound = true;

        if (glossary.is_open()) {

          while (getline(glossary, line)) {
            // Search for the word in the line
            if (line.find(org_file_name) != string::npos) {
              notFound = false;
              cout << "glossary Max for file name" << org_file_name << endl;
              cout << "ERROR: Writing to this file may fail" << endl;
              break;
            }
          }
          if (notFound) {
            ofstream glossary;

            glossary.open("glossary.txt", ios::app);
            glossary << org_file_name << endl;

            glossary.close();
          }
        }
        open_file.close();

        ofstream write_file;
        write_file.open(org_file_name.c_str());
        if (!write_file) {
          cout << "ERROR from Server: unable to open file\n";
          return -1;
        }

        int total_bytes_read = 0;
        int num_bytes_read;
        int receive = 1;
        while (receive != 0) {
          string message;
          char buffer[MAX_LENGTH];
          int num_bytes_read = 0;
          receive = recv(transfer_con, buffer, MAX_LENGTH, 0);
          if (receive < 0) {
            cout << "ERROR from Server: trouble reading from the channel\n";
            return -1;
          }
          if (buffer[0] == '\n') {
            break;
          }

          sleep(2);

          int index = 0;
          while (buffer[index] != '\n') {
            message += buffer[index];
            index++;
          }
          message += '\n';

          num_bytes_read = message.length();
          total_bytes_read += num_bytes_read;

          cout << message;
          write_file << message;

        }

        cout << "Total_bytes_read: " << total_bytes_read << endl << endl;
        write_file.close();

        // Close the File Transfer channel
        close(transfer_con);
      } else if ((string)command_name == "ls") {
        cout << "Working ls..." << endl;

        // Open the File Transfer channel for sending the data
        establish_transfer_channel(transfer_port, transfer_con, transferAddr, connfd);

        ifstream glossary("glossary.txt");

        string current_line;

        if (glossary.is_open()) {

          string file_list_str = "Here's the Files: \n";
          char file_list[MAX_LENGTH];

          for (int i = 0; i < file_list_str.length(); i++) {
            file_list[i] = file_list_str[i];
          }

          cout << "Accessing Files: \n";

          send(transfer_con, file_list, file_list_str.length(), 0);

          while (!glossary.eof()) {
            getline(glossary, current_line);
            current_line += '\n';
            cout << current_line;

            char file_name_char[MAX_LENGTH];
            int i = 0;

            for (; i < current_line.length(); i++) {
              file_name_char[i] = current_line[i];
            }

            if (!glossary.eof()) {
              send(transfer_con, file_name_char, current_line.length(), 0);
              sleep(2);
            }
            else {
              file_name_char[0] = '\n';
              send(transfer_con, file_name_char, 1, 0);
            }
          }
        }
        glossary.close();

        // Close the File Transfer channel
        close(transfer_con);
      } else if ((string)cmd_input == "quit") {
        cout << "Server closing..." << endl;
        close(connfd);
        running = false;
      } else {
        cout << "ERROR: unable to operate on current command\n";
        return -1;
      }
    }
  }
  return 0;
}
