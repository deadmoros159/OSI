#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

char CHILD_PROGRAM_NAME[] = "./child";

int main()
{
    char filename[256];
    {
        const char prompt[] = "Enter filename: ";
        write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);

        ssize_t bytes = read(STDIN_FILENO, filename, sizeof(filename) - 1);
        if (bytes <= 0)
        {
            const char msg[] = "error: failed to read filename\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

        if (filename[bytes - 1] == '\n')
        {
            filename[bytes - 1] = '\0';
        }
        else
        {
            filename[bytes] = '\0';
        }
    }

    int32_t file = open(filename, O_RDONLY);
    if (file == -1)
    {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }


    int parent_to_child[2];
    if (pipe(parent_to_child) == -1)
    {
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        close(file);
        exit(EXIT_FAILURE);
    }

    const pid_t child_pid = fork();

    switch (child_pid)
    {
    case -1:
        {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            close(file);
            close(parent_to_child[0]);
            close(parent_to_child[1]);
            exit(EXIT_FAILURE);
        }
        break;

    case 0:
        {
            close(parent_to_child[1]);
            close(file);
            if (dup2(parent_to_child[0], STDIN_FILENO) == -1)
            {
                const char msg[] = "error: failed to redirect stdin\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
            close(parent_to_child[0]);

            char* const args[] = {"child", NULL};
            execv(CHILD_PROGRAM_NAME, args);

            const char msg[] = "error: failed to exec into child program\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        break;

    default:
        {
            close(parent_to_child[0]);

            char buf[4096];
            ssize_t bytes;


            while ((bytes = read(file, buf, sizeof(buf))) > 0)
            {
                int32_t written = write(parent_to_child[1], buf, bytes);


                if (written != bytes)
                {
                    const char msg[] = "error: failed to write to pipe\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    close(file);
                    close(parent_to_child[1]);
                    wait(NULL);
                    exit(EXIT_FAILURE);
                }
            }

            if (bytes < 0)
            {
                const char msg[] = "error: failed to read from file\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
            }

            close(file);
            close(parent_to_child[1]);

            int status;
            wait(&status);

            if (WIFEXITED(status))
            {
                exit(WEXITSTATUS(status));
            }
            else
            {
                const char msg[] = "error: child terminated abnormally\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
        }
        break;
    }

    return 0;
}
