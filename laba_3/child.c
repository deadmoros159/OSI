#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>

#define SHM_SIZE 4096

typedef struct {
    float result;
    int data_ready;     // флаг, что данные готовы
    int child_done;     // флаг завершения дочернего процесса
} shared_data_t;

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <shm_name> <sem_parent_name> <sem_child_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *shm_name = argv[1];
    char *sem_parent_name = argv[2];
    char *sem_child_name = argv[3];
    
    printf("Дочерний процесс PID: %d\n", getpid());
    printf("Shared memory: %s\n", shm_name);
    printf("Семафор родителя: %s\n", sem_parent_name);
    printf("Семафор ребенка: %s\n", sem_child_name);
    
    // Открываем существующий shared memory объект
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        error_exit("child: shm_open");
    }
    
    // Маппим shared memory
    void *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, 
                         MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd);
        error_exit("child: mmap");
    }
    
    shared_data_t *shared_data = (shared_data_t *)shm_ptr;
    
    // Открываем семафоры
    sem_t *sem_parent = sem_open(sem_parent_name, 0);
    if (sem_parent == SEM_FAILED) {
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        error_exit("child: sem_open parent");
    }
    
    sem_t *sem_child = sem_open(sem_child_name, 0);
    if (sem_child == SEM_FAILED) {
        sem_close(sem_parent);
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        error_exit("child: sem_open child");
    }
    
    
    float sum = 0.0;
    float num;
    int count = 0;
    char line[256];
    
    // Читаем числа построчно (каждая строка может содержать несколько чисел)
    while (fgets(line, sizeof(line), stdin) != NULL) {
        char *token = strtok(line, " \t\n");
        while (token != NULL) {
            // Пытаемся преобразовать токен в число
            char *endptr;
            num = strtof(token, &endptr);
            if (*endptr == '\0') { // Успешное преобразование
                sum += num;
                count++;
                printf("Прочитано число %.2f, сумма = %.2f\n", num, sum);
            }
            token = strtok(NULL, " \t\n");
        }
    }
    
    printf("Всего чисел: %d, сумма = %.2f\n", count, sum);
    
    // Ждем, пока родитель будет готов принять данные
    if (sem_wait(sem_child) == -1) {
        perror("child: sem_wait child");
    } else {
        // Записываем результат в shared memory
        shared_data->result = sum;
        shared_data->data_ready = 1;
        
        printf("Результат записан в shared memory\n");
        
        // Сигнализируем родителю, что данные готовы
        if (sem_post(sem_parent) == -1) {
            perror("child: sem_post parent");
        }
    }
    
    // Очистка
    sem_close(sem_parent);
    sem_close(sem_child);
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    
    printf("Дочерниый процесс завершил работу\n");
    
    return 0;
}