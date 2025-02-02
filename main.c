#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

pid_t pid = 0;

void print_cmd_line(const int argc, char** argv) {
    int i;
    printf("cmd = [%s", argv[0]);
    for(i = 1; i  < argc; ++i) {
        printf(" %s", argv[i]);
    }
    printf("]\n");
}

void notify_user(void) {
    const char bell = '\a';
    while(1) {
        write(STDOUT_FILENO, &bell, sizeof(bell));
        sleep(1);
    }
}

void sigint_handler(int sig) {
    if(pid != 0) {
        char prog_msg[] = "notify: Propagating SIGINT to child process\n";
        write(STDERR_FILENO, prog_msg, sizeof(prog_msg));
        if(kill(pid, sig) == -1) {
            char buf[] = "kill: failed\n";
            write(STDERR_FILENO, buf, sizeof(buf));
            _exit(1);
        }
    }else{
        char msg[] = "notify: Terminated by SIGINT\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(1);
    }
}

int main(const int argc, char** argv) {
    if(argc <= 1) {
        fprintf(stderr, "Usage: %s <prog> [...args]\n", argv[0]);
        exit(1);
    }

    struct sigaction sa = {.sa_handler = sigint_handler};
    sigaction(SIGINT, &sa, NULL);

    const int argc_child = argc - 1;
    char** const argv_child = argv + 1;

    pid = fork();

    if(pid == 0) {
        // child
        execvp(argv_child[0], argv_child);
        fprintf(stderr, "notify: execvp (%s)\n", strerror(errno));
        exit(1);
    }else if(pid > 0) {
        // parent
        printf("notify: launched pid = %d, ", pid);
        print_cmd_line(argc_child, argv_child);
        int wstatus;
        int options = WCONTINUED;
        do {
            const int ret = waitpid(pid, &wstatus, options);
            if(ret == -1 && errno != EINTR) {
                fprintf(stderr, "notify: waitpid (%s)\n", strerror(errno));
                exit(1);
            }

            if(WIFEXITED(wstatus)) {
                printf("notify: %d terminated with status %d ", pid, WEXITSTATUS(wstatus));
                print_cmd_line(argc_child, argv_child);
                break;
            }else if(WIFSIGNALED(wstatus)) {
                printf("notify: %d kill by signal %d ", pid, WTERMSIG(wstatus));
                print_cmd_line(argc_child, argv_child);
                break;
            }else if(WIFSTOPPED(wstatus)) {
                printf("notify: %d stopped\n", pid);
            }else if(WIFCONTINUED(wstatus)) {
                printf("notify: %d continued\n", pid);
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        pid = 0;
        notify_user();
    }else{
        // error
        fprintf(stderr, "notify: fork (%s)\n", strerror(errno));
        exit(1);
    }
    return 0;
}
