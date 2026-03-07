#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef float (*e_func)(int);
typedef float (*area_func)(float, float);

e_func current_e = NULL;
area_func current_area = NULL;

void *e_lib_handle = NULL;
void *area_lib_handle = NULL;

int load_libraries(const char *e_lib_path, const char *e_func_name, 
                   const char *area_lib_path, const char *area_func_name) {
    
    if (e_lib_handle != NULL) {
        dlclose(e_lib_handle);
    }
    
    e_lib_handle = dlopen(e_lib_path, RTLD_LAZY);
    if (!e_lib_handle) {
        fprintf(stderr, "Ошибка загрузки %s: %s\n", e_lib_path, dlerror());
        return -1;
    }
    
    current_e = (e_func)dlsym(e_lib_handle, e_func_name);
    if (!current_e) {
        fprintf(stderr, "Ошибка получения функции %s: %s\n", e_func_name, dlerror());
        return -1;
    }
    
    if (area_lib_handle != NULL) {
        dlclose(area_lib_handle);
    }
    
    area_lib_handle = dlopen(area_lib_path, RTLD_LAZY);
    if (!area_lib_handle) {
        fprintf(stderr, "Ошибка загрузки %s: %s\n", area_lib_path, dlerror());
        return -1;
    }
    
    current_area = (area_func)dlsym(area_lib_handle, area_func_name);
    if (!current_area) {
        fprintf(stderr, "Ошибка получения функции %s: %s\n", area_func_name, dlerror());
        return -1;
    }
    
    return 0;
}

void switch_implementations() {
    static int impl_num = 1;
    
    impl_num = (impl_num == 1) ? 2 : 1;
    
    char e_func_name[20];
    sprintf(e_func_name, "e_impl%d", impl_num);
    
    char area_func_name[20];
    sprintf(area_func_name, "area_impl%d", impl_num);
    
    char e_lib_path[50];
    sprintf(e_lib_path, "./libe_impl%d.so", impl_num);
    
    char area_lib_path[50];
    sprintf(area_lib_path, "./libarea_impl%d.so", impl_num);
    
    if (load_libraries(e_lib_path, e_func_name, area_lib_path, area_func_name) == 0) {
        printf(">>> Переключено на реализацию %d\n", impl_num);
        printf("    e: %s\n", (impl_num == 1) ? "(1+1/x)^x" : "сумма ряда 1/n!");
        printf("    площадь: %s\n", (impl_num == 1) ? "прямоугольник" : "прямоугольный треугольник");
    }
}

int main() {
    char command[256];
    char *token;
    int choice;
    
    if (load_libraries("./libe_impl1.so", "e_impl1", 
                       "./libarea_impl1.so", "area_impl1") != 0) {
        fprintf(stderr, "Не удалось загрузить библиотеки. Сначала выполните 'make'\n");
        return 1;
    }
    
    printf("========================================\n");
    printf("Программа №2 (динамическая загрузка)\n");
    printf("========================================\n");
    printf("Команды:\n");
    printf("  0 - переключить реализации\n");
    printf("  1 x - вычислить e\n");
    printf("  2 a b - вычислить площадь\n");
    printf("  q - выход\n\n");
    printf("Текущая реализация: 1\n");
    printf("  e: (1+1/x)^x\n");
    printf("  площадь: прямоугольник\n\n");
    
    while (1) {
        printf("prog2> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "q") == 0 || strcmp(command, "Q") == 0) {
            break;
        }
        
        token = strtok(command, " ");
        if (token == NULL) continue;
        
        choice = atoi(token);
        
        switch (choice) {
            case 0:
                switch_implementations();
                break;
                
            case 1: {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Ошибка: не указан аргумент x\n");
                    break;
                }
                int x = atoi(token);
                if (current_e) {
                    float result = current_e(x);
                    printf("e = %.6f\n", result);
                }
                break;
            }
            
            case 2: {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Ошибка: не указан аргумент a\n");
                    break;
                }
                float a = atof(token);
                
                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Ошибка: не указан аргумент b\n");
                    break;
                }
                float b = atof(token);
                
                if (current_area) {
                    float result = current_area(a, b);
                    printf("Площадь = %.2f\n", result);
                }
                break;
            }
                
            default:
                printf("Неизвестная команда\n");
                break;
        }
    }
    
    if (e_lib_handle) dlclose(e_lib_handle);
    if (area_lib_handle) dlclose(area_lib_handle);
    
    printf("Программа завершена\n");
    return 0;
}