#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define GPIO_MIN 512
#define GPIO_MAX 538
#define GPIO_PATH_TEMPLATE "/sys/class/gpio/gpio%d"
#define GPIO_DIRECTION_PATH_TEMPLATE "/sys/class/gpio/gpio%d/direction"
#define GPIO_VALUE_PATH_TEMPLATE "/sys/class/gpio/gpio%d/value"

enum Mode { INPUT, OUTPUT };
enum Value { LOW = 0, HIGH = 1 };

void pinMode(int pin, enum Mode mode) {
    if (pin < GPIO_MIN || pin > GPIO_MAX) {
        fprintf(stderr, "Error: GPIO pin must be between %d and %d\n", GPIO_MIN, GPIO_MAX);
        exit(EXIT_FAILURE);
    }

    if (mode != INPUT && mode != OUTPUT) {
        fprintf(stderr, "Error: Invalid mode. Use 'INPUT' or 'OUTPUT'\n");
        exit(EXIT_FAILURE);
    }
    
    // Export GPIO pin if not already exported
    char gpio_path[64];
    snprintf(gpio_path, sizeof(gpio_path), GPIO_PATH_TEMPLATE, pin);
    if (access(gpio_path, F_OK) == -1) {
        int export_fd = open("/sys/class/gpio/export", O_WRONLY);
        if (export_fd == -1) {
            perror("Error opening export file");
            exit(EXIT_FAILURE);
        }
        dprintf(export_fd, "%d", pin);
        close(export_fd);
    }

    char direction_path[64];
    snprintf(direction_path, sizeof(direction_path), GPIO_DIRECTION_PATH_TEMPLATE, pin);
    int direction_fd = open(direction_path, O_WRONLY);
    if (direction_fd == -1) {
        perror("Error opening direction file");
        exit(EXIT_FAILURE);
    }
    const char *mode_str = (mode == INPUT) ? "in" : "out";
    dprintf(direction_fd, "%s", mode_str);
    close(direction_fd);
}

void digitalWrite(int pin, enum Value value) {
    if (pin < GPIO_MIN || pin > GPIO_MAX) {
        fprintf(stderr, "Error: GPIO pin must be between %d and %d\n", GPIO_MIN, GPIO_MAX);
        exit(EXIT_FAILURE);
    }

    if (value != LOW && value != HIGH) {
        fprintf(stderr, "Error: Invalid value. Use 'LOW' or 'HIGH'\n");
        exit(EXIT_FAILURE);
    }

    char value_path[64];
    snprintf(value_path, sizeof(value_path), GPIO_VALUE_PATH_TEMPLATE, pin);
    int value_fd = open(value_path, O_WRONLY);
    if (value_fd == -1) {
        perror("Error opening value file");
        exit(EXIT_FAILURE);
    }
    dprintf(value_fd, "%d", value);
    close(value_fd);
}

int digitalRead(int pin) {
    if (pin < GPIO_MIN || pin > GPIO_MAX) {
        fprintf(stderr, "Error: GPIO pin must be between %d and %d\n", GPIO_MIN, GPIO_MAX);
        exit(EXIT_FAILURE);
    }

    char value_path[64];
    snprintf(value_path, sizeof(value_path), GPIO_VALUE_PATH_TEMPLATE, pin);
    int value_fd = open(value_path, O_RDONLY);
    if (value_fd == -1) {
        perror("Error opening value file");
        exit(EXIT_FAILURE);
    }
    char buf[2];
    ssize_t bytes_read = read(value_fd, buf, sizeof(buf));
    if (bytes_read == -1) {
        perror("Error reading value file");
        close(value_fd);
        exit(EXIT_FAILURE);
    }
    close(value_fd);
    return atoi(buf);
}

void blink(int pin, float freq, int duration) {
    pinMode(pin, OUTPUT);

    int half_period_usec = 1000000 / (2 * freq); // Calculate half period in microseconds

    for (int i = 0; i < duration * freq * 2; i++) {
        digitalWrite(pin, i % 2 == 0 ? HIGH : LOW); // Toggle pin state
        usleep(half_period_usec); // Wait for half period
    }
    digitalWrite(pin, LOW);
}
