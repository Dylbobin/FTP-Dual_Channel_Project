Names - Emails:
  Anne Edwards - aedwards20@csu.fullerton.edu
  Dylan Silva - dylansilva@csu.fullerton.edu
  Braulio Martin - brauliom01@csu.fullerton.edu

Programming Language: C++

How to run:
To run the program, make sure that client.cpp, server.cpp, parse_lib.h/.cpp, and glossary.txt are all in the same directory. On one terminal, run g++ server.cpp, then run ./a.out <port number>. Once that is running, in a separate terminal run g++ client.cpp, then run ./a.out <some website domain> <port number entered when running server.cpp>. The terminal running client.cpp will then prompt you for input based on the instructions printed on the terminal window. To quit the program, simply enter "quit" (without quotations) in the client side terminal window and both programs will terminate.

Special Note:
We have included glossary.txt to keep track of the files that have been created on the server side. It is necessary to have for "ls" to work. As the glossary only accounts for documents on the server side after put, it is necessary for at least one call to "put" before calling "ls." Any transferred file has a (1) appended to the name to distinguish it from the original file. However, if you wish to resend the file (i.e. test.txt), test(1).txt must be deleted beforehand if it already exists. 

If problems receiving the transport port number occur, continue trying until port is sent to server.  This is an expected error for this program, likely due to heavy traffic on certain ISP networks.  This will occur within each of the different functions (GET, PUT, LS)