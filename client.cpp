#include "parse_lib.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
using namespace std;

#define MSG_SIZE 100
#define MAX_LENGTH 100

/* Prints the different commands the user can enter GET, POST, ls, quit and returns string representing command for main to call */
string ftp_commands() {
  string ftp_command;
  cout << "Here is a list of commands that you can call:\n";
  cout << "  get <filename>\n";
  cout << "  put <filename>\n";
  cout << "  ls\n";
  cout << "  quit\n";
  cout << "ftp> ";
  getline(cin, ftp_command, '\n');
  return ftp_command;
}

char *resolve_ip(const char *host_name, const char *port) {
  // DNS lookup function
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // use either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // use TCP
  int status = getaddrinfo(host_name, port, &hints, &res);
  if (status != 0) {
    std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
    return 0;
  }

  char ipstr[INET6_ADDRSTRLEN];
  void *addr;
  if (res->ai_family == AF_INET) { // IPv4
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    addr = &(ipv4->sin_addr);
  } else { // IPv6
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
    addr = &(ipv6->sin6_addr);
  }

  inet_ntop(res->ai_family, addr, ipstr, sizeof ipstr);

  cout << "HOST: " << host_name << endl << "IP: " << ipstr << std::endl;

  freeaddrinfo(res);
  char *ip_ptr = ipstr;

  return ip_ptr;
}

void establish_connection(int &trans_con, sockaddr_in &transfer_address,
                          socklen_t transfer_len, int transfer_port,
                          int &trans_channel, int cli_con) {
  trans_con = socket(AF_INET, SOCK_STREAM, 0);
  if (trans_con < 0) {
    cout << "ERROR from Client: unable to open transfer socket\n";
    exit(-1);
  }
  memset(&transfer_address, 0, sizeof(transfer_address));
  transfer_address.sin_family = AF_INET;

  transfer_address.sin_addr.s_addr = INADDR_ANY;

  // Convert the port number to network representation
  transfer_address.sin_port = htons(transfer_port);

  // Bind the ephemeral port
  bind(trans_con, (sockaddr *)&transfer_address, sizeof(transfer_address));

  // Get the new socket number
  getsockname(trans_con, (struct sockaddr *)&transfer_address, &transfer_len);

  // Send the ephemeral port number
  char trans_port_msg[MAX_LENGTH];

  snprintf(trans_port_msg, MAX_LENGTH, "%d", ntohs(transfer_address.sin_port));
  // Send the transfer port number to the server over the command channel
  char *content_ptr = &trans_port_msg[0];
  int total_bytes_sent = 0;
  int num_bytes_sent = 0;
  while (total_bytes_sent != sizeof(trans_port_msg)) {
    num_bytes_sent = send(cli_con, content_ptr, MAX_LENGTH, 0);
    if (num_bytes_sent < 0) {
      cout << "ERROR from Client: unable to send file transport port number\n";
      exit(-1);
    }
    total_bytes_sent += num_bytes_sent;
  }
  sleep(1);

  if (listen(trans_con, 50) < 0) {
    cout << "ERROR from client: Unable to listen\n";
    exit(-1);
  }

  // Allow the server to connect and have their details stored
  if ((trans_channel = accept(trans_con, (sockaddr *)&transfer_address,
                              &transfer_len)) < 0) {
    cout << "ERROR from Client: Unable to accept on the transfer channel\n";
    exit(-1);
  }
}

int main(int argc, char **argv) {
  int port = -1;
  int transfer_port = 0;

  // The file descriptor representing the connection to the client
  int cli_con = -1;
  int trans_con = -1;     
  int trans_channel = -1; 

  // The structures representing the server address
  sockaddr_in server_address;
  sockaddr_in transfer_address;

  // Stores the size of the client's address
  socklen_t serv_len = sizeof(server_address);
  socklen_t trans_len = sizeof(transfer_address);

  // Make sure that the user has provided the port number of the server
  if (argc < 3) {
    cout << "ERROR: Please provide the name of the server the port number for "
            "the command channel\n";
    return -1;
  }

  // Get the port number
  port = atoi(argv[2]);

  // Make sure that the port number is valid
  if (port < 0 || port > 65535 || transfer_port < 0 || transfer_port > 65535) {
    cout << "ERROR: Invalid port number\n";
    return -1;
  }

  // Open the channel for the client to listen on
  cli_con = socket(AF_INET, SOCK_STREAM, 0);
  if (cli_con < 0) {
    cout << "ERROR: Unable to open socket\n";
    return -1;
  }

  // Set the structure to all zeros
  memset(&server_address, 0, sizeof(server_address));

  // Set the server family
  server_address.sin_family = AF_INET;

  // Convert the port number to network representation
  server_address.sin_port = htons(port);

  const char *host_name = argv[1];
  const char *port_to_read = argv[2];

  resolve_ip(host_name, port_to_read);

  // Connect to the Communication Channel
  int success_connect =
      connect(cli_con, (sockaddr *)&server_address, sizeof(sockaddr));
  if (success_connect < 0) {
    cout << "ERROR from Client: Unable to connect to server\n";
    return -1;
  }

  // Send an intital message as proof of connection
  string content = "Hello from the client\n";
  char *content_ptr = &content[0];

  int total_bytes_sent = 0;
  int num_bytes_sent = 0;
  while (total_bytes_sent != sizeof(content)) {
    num_bytes_sent = write(cli_con, content_ptr + total_bytes_sent,
                           sizeof(content) - total_bytes_sent);
    if (num_bytes_sent < 0) {
      cout << "ERROR from Client: unable to send greeting\n";
      return -1;
    }
    total_bytes_sent += num_bytes_sent;
  }

  // Get user input, individual command, individual filename
  string cmd_input = "temp";
  while (cmd_input != "quit") {
    string cmd_input = ftp_commands();            
    string file_name = parse_filename(cmd_input); 
    string operation =
        parse_operation(cmd_input); 

    cout << "\n\nYou chose: " << cmd_input << endl;

    // Send the info to the server
    content_ptr = &cmd_input[0];
    total_bytes_sent = 0;
    num_bytes_sent = 0;
    
    // Send the command and filename to the server through the command channel
    while (total_bytes_sent != sizeof(cmd_input)) {
      num_bytes_sent = write(cli_con, content_ptr + total_bytes_sent,
                             sizeof(cmd_input) - total_bytes_sent);
      if (num_bytes_sent < 0) {
        cout << "ERROR: unable to send ftp command\n";
        return -1;
      }
      total_bytes_sent += num_bytes_sent;
    }

    // Do the actions based on the input
    if ((string)operation == "put") {
      cout << "  The name of the file is " << file_name << endl;
      cout << "  The chosen operation is " << operation << endl;

    // Open the transfer channel for sending the data
      establish_connection(trans_con, transfer_address, trans_len,
                           transfer_port, trans_channel, cli_con);

    // Open the file
      ifstream file;
      file.open(file_name.c_str());
      if (!file) {
        cout << "ERROR: Client unable to open file\n";
        return -1;
      }

     // Send the file
      total_bytes_sent = 0;
      while (!file.eof()) {
        string message;
        getline(file, message, '\n');
        message += '\n';

     // Turn the string into a character array for the transfer
        num_bytes_sent = 0;
        char message_to_send[1000];
        int i = 0;
        for (; i < message.length(); i++) {
          message_to_send[i] = message[i];
        }

        // Send the buffered data
        if (!file.eof()) {
          send(trans_channel, message_to_send, message.length(), 0);
          sleep(2);
          num_bytes_sent = message.length();
          if (num_bytes_sent < 0) {
            cout << "ERROR: unable to send ftp command\n";
            return -1;
          }
          total_bytes_sent += num_bytes_sent;
        } else {
          message_to_send[0] = '\n';
          send(trans_channel, message_to_send, 1, 0);
        }
      }
      file.close();
      cout << "Sent " << total_bytes_sent << " bytes of data\n\n\n";

      // Close the transfer channel
      close(trans_channel);
    } else if ((string)operation == "get") {
      cout << "Pending Get..." << endl;

      // Open the transfer channel for reception of data
      establish_connection(trans_con, transfer_address, trans_len,
                           transfer_port, trans_channel, cli_con);

      // Check to see if the file already exists -> if yes, append a (1) to the
      // End of the name of the copy
      ifstream open_file;
      string fileName;
      open_file.open(file_name);
      if (open_file) {
        // The file already exists -> append a (1)
        int i = 0;
        while (i < file_name.length() - 1) {
          char character = file_name[i];
          if (character != '.') {
            fileName += character;
            i++;
          } else {
            fileName += "(1)";
            fileName += file_name[i];
            i++;
            fileName += file_name[i];
            i++;
            fileName += file_name[i];
            i++;
            fileName += file_name[i];
            break;
          }
        }
      } else {
        fileName = file_name;
      }

      // Open the actual file to write in
      ofstream write_file;
      write_file.open(fileName);
      int total_bytes_read = 0;
      int num_bytes_read;
      int receive = 1;
      while (receive != 0) {
        string message;
        num_bytes_read = 0;
        char buffer[MAX_LENGTH];
        receive = recv(trans_channel, buffer, MAX_LENGTH, 0);
        if (receive < 0) {
          cout << "ERROR: trouble reading the file\n";
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
        write_file << message; 
      }

      cout << "Total_bytes_read: " << total_bytes_read << endl << endl;
      write_file.close();

      // Close the transfer channel
      close(trans_channel);
    } else if ((string)cmd_input == "ls") {
      cout << "Pending ls...." << endl;

      // Open the tranfer channel for reception
      establish_connection(trans_con, transfer_address, trans_len,
                           transfer_port, trans_channel, cli_con);

      char file_header[MAX_LENGTH]; 
      recv(trans_channel, file_header, 19, 0);

      int num_recv = sizeof(file_header);
      if (num_recv < 0) {
        cout << "ERROR: unable to receive file transport port number\n";
        return -1;
      }

      int i = 0;
      while (file_header[i] != '\n') {
        cout << file_header[i];
        i++;
      }
      cout << endl;

      int total_bytes_read = 0;
      int num_bytes_read = 0;
      int receive = 1;

      while (receive != 0) {
        string message;
        num_bytes_read = 0;
        char buffer[MAX_LENGTH];
        receive = recv(trans_channel, buffer, MAX_LENGTH, 0);
        sleep(2);
        if (receive < 0) {
          cout << "ERROR: trouble reading the file\n";
          return -1;
        }

        if (buffer[0] == '\n') {
          break;
        }
        int index = 0;

        while (buffer[index] != '\n') {
          cout << buffer[index];
          index++;
        }
        cout << endl;
      }

      // close the transfer channel
      close(trans_channel);
    } else if ((string)cmd_input == "quit") {
      cout << "Quitting..." << endl;
      close(cli_con);
      return 0;
    } else {
      cout << "ERROR: could not understand or route command" << endl;
      return -1;
    }
  }

  return 0;
}
