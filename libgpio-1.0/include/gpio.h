#ifndef GPIO_H
#define GPIO_H

#define GPIO_MIN 512
#define GPIO_MAX 538
#define GPIO_PATH_TEMPLATE "/sys/class/gpio/gpio%d"
#define GPIO_DIRECTION_PATH_TEMPLATE "/sys/class/gpio/gpio%d/direction"
#define GPIO_VALUE_PATH_TEMPLATE "/sys/class/gpio/gpio%d/value"

enum Mode { INPUT, OUTPUT };
enum Value { LOW = 0, HIGH = 1 };

void pinMode(int pin, enum Mode mode);
void digitalWrite(int pin, enum Value value);
int digitalRead(int pin);
void blink(int pin, float freq, int duration);

#endif /* GPIO_H */

