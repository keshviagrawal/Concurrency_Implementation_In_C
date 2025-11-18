# Concurrency Control in C — Bakery Algorithm

This project is a small demonstration of how to achieve safe mutual exclusion between multiple threads in C using the classic **Bakery Algorithm**. It shows how threads can coordinate fairly without using built-in mutex primitives.

## Project Overview

### **`bakery/`**
Contains the implementation of Lamport’s Bakery Algorithm.  
Each thread takes a numbered “ticket” and waits its turn, ensuring fairness and preventing race conditions.

### **`main.c`**
The driver program that:
- Creates multiple threads  
- Makes them compete for a shared resource  
- Uses the Bakery Algorithm to control entry into the critical section  

A simple and clear way to observe synchronization in action.
