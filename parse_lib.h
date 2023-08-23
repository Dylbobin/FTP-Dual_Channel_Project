#include <iostream>
#include <ostream>
#include <string.h>
#include <unistd.h> /* Contains standard unix functions */
#include <sys/socket.h>
#include "parse_lib.h"
using namespace std;

  /* for separating the filename from the ftp command
   called when command was ftp> get <filename> or ftp> put <filename> */
string parse_filename(string ftp_command) {
  string file_name;
  int location = 0;
  char character = ftp_command[location];
  while (character != ' ') {
    character = ftp_command[location];
    location++;
  }
  for (; location < ftp_command.length(); location++) {
    file_name += ftp_command[location];
  }
  return file_name;
}

  /* for separating the command from the filename
  called when command was ftp> get <filename> or ftp> put <filename> */
string parse_operation(string ftp_command) {
  string operation;
  int location = 0;
  char character = ftp_command[location];
  while (character != ' ') {
    operation += character;
    location++;
    character = ftp_command[location];
  }
  return operation;
}
