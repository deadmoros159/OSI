#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#define SHM_SIZE 4096

typedef struct {
    float result;
    int data_ready;     // флаг, что данные готовы
    int child_done;     // флаг завершения дочернего процесса
} shared_data_t;

void error_exit(const char *text) {
    perror(text);
    exit(EXIT_FAILURE);
}

void cleanup(const char *shm_name, const char *sem_parent_name, 
            const char *sem_child_name, void *shm_ptr, int shm_fd) {
    if (shm_ptr != NULL && shm_ptr != MAP_FAILED) {
        munmap(shm_ptr, SHM_SIZE);
    }
    if (shm_fd >= 0) {
        close(shm_fd);
    }
    if (shm_name != NULL) {
        shm_unlink(shm_name);
    }
    if (sem_parent_name != NULL) {
        sem_unlink(sem_parent_name);
    }
    if (sem_child_name != NULL) {
        sem_unlink(sem_child_name);
    }
}

int main() {
    pid_t pid;
    char filename[256];
    char shm_name[256];
    char sem_parent_name[256];
    char sem_child_name[256];
    
    // Генерируем уникальные имена на основе PID родителя
    sprintf(shm_name, "/shm_%d", getpid());
    sprintf(sem_parent_name, "/sem_parent_%d", getpid());
    sprintf(sem_child_name, "/sem_child_%d", getpid());
    
    printf("Родительский процесс PID: %d\n", getpid());
    printf("Имя shared memory: %s\n", shm_name);
    printf("Имя семафора родителя: %s\n", sem_parent_name);
    printf("Имя семафора ребенка: %s\n", sem_child_name);
    
    // Ввод имени файла
    printf("Введите имя файла: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        error_exit("fgets");
    }
    
    // Удаляем символ новой строки
    filename[strcspn(filename, "\n")] = '\0';
    
    if (strlen(filename) == 0) {
        fprintf(stderr, "Введите имя файла\n");
        exit(EXIT_FAILURE);
    }
    
    // Создаем shared memory объект
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        error_exit("shm_open");
    }
    
    // Устанавливаем размер
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        cleanup(shm_name, NULL, NULL, NULL, shm_fd);
        error_exit("Уменьшите файл");
    }
    
    // Маппим shared memory в адресное пространство
    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, 
                         MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        cleanup(shm_name, NULL, NULL, NULL, shm_fd);
        error_exit(MAP_FAILED);
    }
    
    // Инициализируем разделяемую память
    shared_data_t *shared_data = (shared_data_t *)shm_ptr;
    shared_data->data_ready = 0;
    shared_data->child_done = 0;
    
    // Создаем семафоры
    sem_t *sem_parent = sem_open(sem_parent_name, O_CREAT, 0666, 0);
    if (sem_parent == SEM_FAILED) {
        cleanup(shm_name, NULL, NULL, shm_ptr, shm_fd);
        error_exit("sem_open parent");
    }
    
    sem_t *sem_child = sem_open(sem_child_name, O_CREAT, 0666, 1); // Начинаем с 1, чтобы ребенок мог писать
    if (sem_child == SEM_FAILED) {
        sem_unlink(sem_parent_name);
        cleanup(shm_name, sem_parent_name, NULL, shm_ptr, shm_fd);
        error_exit("sem_open child");
    }
    
    // Закрываем семафоры в родителе (они все равно будут открыты в ребенке)
    sem_close(sem_parent);
    sem_close(sem_child);
    
    // Создаем дочерний процесс
    pid = fork();
    if (pid == -1) {
        cleanup(shm_name, sem_parent_name, sem_child_name, shm_ptr, shm_fd);
        error_exit("fork");
    }
    
    if (pid == 0) {
        // Дочерний процесс
        // Перенаправляем stdin на файл
        FILE *file = freopen(filename, "r", stdin);
        if (file == NULL) {
            fprintf(stderr, "Не удалось открыть файл ребенка %s\n", filename);
            exit(EXIT_FAILURE);
        }
        
        // Запускаем программу ребенка
        execl("./child", "child", shm_name, sem_parent_name, sem_child_name, NULL);
        
        // Если execl вернулся, значит произошла ошибка
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Родительский процесс
        printf("Дочерний процесс создан (PID: %d)\n", pid);
        
        // Открываем семафоры для родителя
        sem_parent = sem_open(sem_parent_name, 0);
        if (sem_parent == SEM_FAILED) {
            cleanup(shm_name, sem_parent_name, sem_child_name, shm_ptr, shm_fd);
            error_exit("sem_open parent in parent process");
        }
        
        // Ждем сигнала от ребенка о готовности данных
        if (sem_wait(sem_parent) == -1) {
            sem_close(sem_parent);
            cleanup(shm_name, sem_parent_name, sem_child_name, shm_ptr, shm_fd);
            error_exit("sem_wait parent");
        }
        
        // Читаем результат из shared memory
        if (shared_data->data_ready) {
            printf("Получен результат: %.2f\n", shared_data->result);
        }
        
        // Сигнализируем ребенку, что данные прочитаны
        sem_post(sem_child);
        
        // Ждем завершения дочернего процесса
        wait(NULL);
        
        // Закрываем семафоры
        sem_close(sem_parent);
        
        // Очищаем ресурсы
        cleanup(shm_name, sem_parent_name, sem_child_name, shm_ptr, shm_fd);
        
    }
    
    return 0;
}