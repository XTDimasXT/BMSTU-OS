#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define LOCKFILE "/var/run/daemon.pid"
#define LOCKMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define SLEEP_TIME 30

sigset_t mask;

int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
}

void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        syslog(LOG_ERR, "%s: невозможно получить максимальный номер дескриптора ", cmd);
        exit(1);
    }

    if ((pid = fork()) < 0)
    {
        syslog(LOG_ERR, "%s: ошибка вызова функции fork", cmd);
        exit(1);
    }
    else if (pid != 0)
        exit(0);

    if (setsid() == -1)
    {
        syslog(LOG_ERR, "setsid error");
        exit(1);
    }

    sa.sa_handler = SIG_IGN;
    if (sigemptyset(&sa.sa_mask) == -1)
    {
        syslog(LOG_ERR, "sigemptyset error");
        exit(1);
    }

    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        syslog(LOG_ERR, "%s: невозможно игнорировать сигнал SIGHUP", cmd);
        exit(1);
    }

    if (chdir("/") < 0)
    {
        syslog(LOG_ERR, "%s: невозможно сделать текущим рабочим каталогом /", cmd);
        exit(1);
    }

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "ошибочные файловые дескрипторы %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}

int already_running(void)
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0)
    {
        syslog(LOG_ERR, "невозможно открыть %s: %s\n", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "невозможно установить блокировку на %s: %s\n", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (ftruncate(fd, 0) == -1)
    {
        syslog(LOG_ERR, "ftruncate error \n");
        exit(1);
    }
    if (sprintf(buf, "%ld", (long)getpid()) < 0)
    {
        syslog(LOG_ERR, "sprintf error \n");
        exit(1);
    }

    write(fd, buf, strlen(buf) + 1);
    return 0;
}

void reread(void)
{
    FILE *fd;
    int pid;

    if ((fd = fopen("/var/run/daemon.pid", "r")) == NULL)
    {
        syslog(LOG_ERR, "не получается открыть /var/run/daemon.pid.");
        exit(1);
    }

    fscanf(fd, "%d", &pid);
    fclose(fd);

    syslog(LOG_INFO, "Pid прочитанный из файла: %d", pid);
}

void *thr_fn(void *arg)
{
    int err, signo;

    syslog(LOG_INFO, "%s tid=%d\n", (char *)arg, gettid());
    for (;;)
    {

        err = sigwait(&mask, &signo);
        if (err != 0)
        {
            syslog(LOG_ERR, "ошибка вызова функции sigwait\n");
            exit(1);
        }

        switch (signo)
        {
        case SIGHUP:
            syslog(LOG_INFO, "Чтение конфигурационного файла.\n");
            reread();
            break;
        case SIGTERM:
            syslog(LOG_INFO, "получен сигнал SIGTERM; выход.\n");
            exit(0);
        case SIGINT:
            syslog(LOG_INFO, "получен сигнал SIGINT;");
            pthread_exit(NULL);
        default:
            syslog(LOG_INFO, "получен непредвиденный сигнал %d\n", signo);
            break;
        }
    }
    return (void *)0;
}

int main(int argc, char *argv[])
{
    int err;
    int status;
    pthread_t tid_arr[2];
    char *thread_args[2] = {"aaa", "bbb"};
    char *cmd;
    struct sigaction sa;
    pthread_attr_t attr_arr[2];

    if ((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;

    daemonize(cmd);

    if (already_running())
    {
        syslog(LOG_ERR, "демон уже запущен\n");
        exit(1);
    }

    sa.sa_handler = SIG_DFL;
    if (sigemptyset(&sa.sa_mask) < 0)
    {
        syslog(LOG_ERR, "sigemptyset error");
        exit(1);
    }
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0)
        syslog(LOG_ERR, "возможно восстановить действие SIG_DFL для SIGHUB\n");

    if (sigfillset(&mask) < 0)
        syslog(LOG_ERR, "sigfillset error");

    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        syslog(LOG_ERR, "Ошибка выполнения операции SIG_BLOC");

    for (int i = 0; i < 2; ++i)
    {
        if (pthread_attr_init(&attr_arr[i]) != 0)
            syslog(LOG_ERR, "Ошибка выполнения операции pthread_attr_init");

        err = pthread_create(&tid_arr[i], &attr_arr[i], thr_fn, thread_args[i]);
        if (err != 0)
            syslog(LOG_ERR, "невозможно создать поток\n");
        else
            syslog(LOG_INFO, "Новый поток создан\n");
    }

    void *status_addr = NULL;

    for (int i = 0; i < 2; ++i)
    {
        status = pthread_join(tid_arr[i], (void *)&status_addr);
        if (status != 0)
        {
            syslog(LOG_ERR, "Ошибка pthread_join, status = %d", status);
            exit(0);
        }
    }

    time_t timestamp;
    struct tm *time_info;
    for (;;)
    {
        sleep(SLEEP_TIME);
        time(&timestamp);
        time_info = localtime(&timestamp);
        syslog(LOG_INFO, "%s", asctime(time_info));
    }

    for (int i = 0; i < 2; ++i)
        if (pthread_attr_destroy(&attr_arr[i]) != 0)
            syslog(LOG_ERR, "Ошибка выполнения операции pthread_attr_destroy");
}