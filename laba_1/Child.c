#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#define MAX_NUMBERS 100
#define MAX_NUM_LENGTH 64
#define BUFFER_SIZE 4096


bool parse_float(const char* str, float* result)
{
    char* endptr;
    float value = strtof(str, &endptr);
    

    if (endptr == str)
    {
        return false;
    }
    
    while (*endptr != '\0')
    {
        if (!isspace(*endptr))
        {
            return false;
        }
        endptr++;
    }
    
    if (value == HUGE_VALF || value == -HUGE_VALF || value == 0.0f)
    {
        return true;
    }
    
    *result = value;
    return true;
}

int32_t float_to_string(float num, char* str, int precision)
{
    if (isnan(num))
    {
        return snprintf(str, MAX_NUM_LENGTH, "nan");
    }
    
    if (isinf(num))
    {
        if (num > 0)
            return snprintf(str, MAX_NUM_LENGTH, "inf");
        else
            return snprintf(str, MAX_NUM_LENGTH, "-inf");
    }
    

    return snprintf(str, MAX_NUM_LENGTH, "%.*f", precision, num);
}

void process_line(const char* line, int32_t length)
{
    float numbers[MAX_NUMBERS];
    int32_t count = 0;
    
    char num_buffer[MAX_NUM_LENGTH];
    int32_t num_index = 0;
    
    for (int32_t i = 0; i < length; ++i)
    {
        char c = line[i];
        
        if ((c >= '0' && c <= '9') || 
            c == '-' || c == '+' || 
            c == '.' || 
            c == 'e' || c == 'E')
        {
            if (num_index < MAX_NUM_LENGTH - 1)
            {
                num_buffer[num_index++] = c;
            }
        }
        else if (isspace(c))
        {
            if (num_index > 0)
            {
                num_buffer[num_index] = '\0';
                float num;
                if (parse_float(num_buffer, &num) && count < MAX_NUMBERS)
                {
                    numbers[count++] = num;
                }
                num_index = 0;
            }
        }
    }

    if (num_index > 0)
    {
        num_buffer[num_index] = '\0';
        float num;
        if (parse_float(num_buffer, &num) && count < MAX_NUMBERS)
        {
            numbers[count++] = num;
        }
    }
    
    float sum = 0.0f;
    for (int32_t i = 0; i < count; ++i)
    {
        sum += numbers[i];
    }
    
    char result[64];
    int32_t len = float_to_string(sum, result, 6);
    
    char output[128];
    int32_t output_len = snprintf(output, sizeof(output), 
                                  "Sum of %d numbers: %s\n", count, result);
    
    write(STDOUT_FILENO, output, output_len);
}

int main(void)
{
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    int32_t line_length = 0;
    ssize_t bytes;
    
    int32_t line_number = 0;
    
    while ((bytes = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0)
    {
        for (int32_t i = 0; i < bytes; ++i)
        {
            if (buffer[i] == '\n')
            {
                if (line_length > 0)
                {
                    line_number++;
                    char line_header[32];
                    int32_t header_len = snprintf(line_header, sizeof(line_header), 
                                                  "Line %d: ", line_number);
                    write(STDOUT_FILENO, line_header, header_len);
                    
                    process_line(line, line_length);
                    line_length = 0;
                }
            }
            else
            {
                if (line_length < BUFFER_SIZE - 1)
                {
                    line[line_length++] = buffer[i];
                }
            }
        }
    }
    
    if (bytes < 0)
    {
        const char msg[] = "error: failed to read from stdin\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    
    return 0;
}
