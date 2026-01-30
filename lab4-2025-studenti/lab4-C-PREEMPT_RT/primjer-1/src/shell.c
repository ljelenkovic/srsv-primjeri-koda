#include <unistd.h>
#include <sys/wait.h>

int sys_waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options, void*);

int main(void)
{
    char buffer[256];
    write(STDOUT_FILENO, "SRSV, Lab 4 - simple program runner shell\n", 42);
    while(1)
    {
        write(STDOUT_FILENO, "$ ", 2);
        int cmd_len = read(STDIN_FILENO, buffer, 255);
        buffer[cmd_len - 1] = 0;
        int pid = fork();
        if(pid == 0) 
        {
            char *argv[] = {buffer, NULL};
            execve(buffer, argv, NULL);
            break;
        }
        siginfo_t info;
        sys_waitid(P_ALL, 0, &info, WEXITED, NULL);
    }
    _exit(0);
}