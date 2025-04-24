#include <stdio.h>
#include <gpio.h>

int main() {
    // GPIO pins
    int outputPin = 517; // Change this to your desired output pin number
    int inputPin = 518;  // Change this to your desired input pin number
    
    // Set output pin value
    pinMode(outputPin, OUTPUT);
    digitalWrite(outputPin, HIGH); // Set output pin to HIGH
    
    // Read input pin value
    pinMode(inputPin, INPUT);
    int inputValue = digitalRead(inputPin);
    // Print input pin value
    printf("Value read from input pin %d: %d\n", inputPin, inputValue);
	
    // Example usage
    blink(529, 0.5, 5); // Blink pin 17 at 0.5 Hz for 5 seconds

    return 0;
}

