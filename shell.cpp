#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

#define MAX_LINE 80 // max length command

int main() {
     char line[MAX_LINE] = {};
     char history[MAX_LINE] = {};
     int should_run = 1; // flag

     while (should_run) {
          cout << ("osh> ") << flush;
          cin.getline(line, MAX_LINE);
          vector<char *> args;

          // save history (before tokenizer separates spaces)
          if (strcmp(line, "!!") != 0 && strlen(line) > 0) {
               strcpy(history, line);
          }

          // check history
          if (strcmp(line, "!!") == 0) {
               if (strlen(history) == 0) {
                    cout << "no commands in history" << endl;
                    continue;
               }
               cout << history << endl;
               strcpy(line, history);
          }

          // tokenizer
          char *token = strtok(line, " \n");
          while (token != NULL) {
               args.push_back(token);
               token = strtok(NULL, " \n");
          }
          args.push_back(NULL);

          // prevent crash if user only presses enter
          if (args.size() == 1) {
               continue;
          }

          // dbugger print, checks args arr contents
          // for (size_t i = 0; i < args.size() - 1; i++) {
          //      cout << "args[" << i << "]" << args[i] << endl;
          // }

          // check for &
          bool run_bg = false;
          int lastindex = args.size() - 2;
          if (lastindex >= 0 && strcmp(args[lastindex], "&") == 0) {
               run_bg = true;
               // replace & to NULL to prevent errors in execvp
               args[lastindex] = NULL;
          }

          // check for input/output operator

          // keep track of operation, 0 = none, 1 = input, 2 = output
          int ioOperation = 0;
          string filename = "";
          int pipeIndex = 0;
          for (int i = 1; i < args.size() - 1; i++) {
               if (args[i] != nullptr && strcmp(args[i], "<") == 0 &&
                   args[i + 1] != nullptr) {
                    ioOperation = 1;
                    filename = args[i + 1];
                    args[i] = NULL;
                    break;
               } else if (args[i] != nullptr && strcmp(args[i], ">") == 0 &&
                          args[i + 1] != nullptr) {
                    ioOperation = 2;
                    filename = args[i + 1];
                    args[i] = NULL;
                    break;
               } else if (args[i] != nullptr && strcmp(args[i], "|") == 0 &&
                          args[i + 1] != nullptr) { // check for pipe
                    pipeIndex = i + 1;
                    args[i] = NULL;
                    break;
               }
          }

          // forking
          int rc = fork();
          if (rc < 0) {
               cerr << "fork failure" << endl;
          } else if (rc == 0) {
               // success child fork

               // operation, 0 = none, 1 = input, 2 = output
               if (ioOperation == 2) { // output
                    int fd = open(filename.c_str(),
                                  O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                         cerr << "failed to open file: " << filename << endl;
                         exit(1);
                    }

                    dup2(fd, STDOUT_FILENO);
                    close(fd);

               } else if (ioOperation == 1) { // input
                    int fd = open(filename.c_str(), O_RDONLY);

                    if (fd < 0) {
                         cerr << "failed to open file: " << filename << endl;
                         exit(1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
               }

               if (pipeIndex > 0) {
                    int pipefd[2];
                    pipe(pipefd);

                    int rc2 = fork();
                    if (rc2 == 0) {

                         dup2(pipefd[0], STDIN_FILENO);
                         close(pipefd[0]);
                         close(pipefd[1]);

                         if (execvp(args[pipeIndex], &args[pipeIndex]) == -1) {
                              cout << "no command found" << endl;
                              exit(1);
                         }

                    } else {
                         dup2(pipefd[1], STDOUT_FILENO);
                         close(pipefd[0]);
                         close(pipefd[1]);
                    }
               }

               if (execvp(args[0], args.data()) == -1) {
                    cout << "no command found" << endl;
                    exit(1);
               }
          } else {
               if (!run_bg) {
                    wait(NULL);
               }
               continue;
          }
     }
     return 0;
}
