#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "e_calculator.h"
#include "area_calculator.h"

int main() {
    char command[256];
    char *token;
    int choice;
    
    printf("========================================\n");
    printf("Программа №1 (статическая линковка)\n");
    printf("========================================\n");
    printf("Команды:\n");
    printf("  1 x - вычислить e (реализация 1)\n");
    printf("  2 a b - вычислить площадь прямоугольника\n");
    printf("  q - выход\n\n");
    
    while (1) {
        printf("prog1> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "q") == 0 || strcmp(command, "Q") == 0) {
            break;
        }
        
        token = strtok(command, " ");
        if (token == NULL) continue;
        
        choice = atoi(token);
        
        switch (choice) {
            case 1: {
                token = strtok(NULL, " ");
                if (token == NULL) {
                    printf("Ошибка: не указан аргумент x\n");
                    break;
                }
                int x = atoi(token);
                float result = e_impl1(x);
                printf("e = %.6f (приближение 1: (1+1/%d)^%d)\n", result, x, x);
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
                
                float result = area_impl1(a, b);
                printf("Площадь прямоугольника = %.2f\n", result);
                break;
            }
            
            default:
                printf("Неизвестная команда\n");
                break;
        }
    }
    
    printf("Программа завершена\n");
    return 0;
}