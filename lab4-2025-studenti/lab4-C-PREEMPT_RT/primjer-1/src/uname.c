#include <unistd.h>
#include <sys/utsname.h>

int main(void)
{
    struct utsname info;
    int l;

    uname(&info);
    
    l = 0;
    while (info.sysname[l++]);
    write(STDOUT_FILENO, info.sysname, l);
    write(STDOUT_FILENO, " ", 1);

    l = 0;
    while (info.nodename[l++]);
    write(STDOUT_FILENO, info.nodename, l);
    write(STDOUT_FILENO, " ", 1);
    
    l = 0;
    while (info.release[l++]);
    write(STDOUT_FILENO, info.release, l);
    write(STDOUT_FILENO, " ", 1);
    
    l = 0;
    while (info.version[l++]);
    write(STDOUT_FILENO, info.version, l);
    write(STDOUT_FILENO, " ", 1);
    
    l = 0;
    while (info.machine[l++]);
    write(STDOUT_FILENO, info.machine, l);
    write(STDOUT_FILENO, "\n", 1);

    return 0;
}