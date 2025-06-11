# 🧠🔄 Lab #4 - Mecanismos de Sincronización en Sistemas Operativos

> Universidad de Antioquia - Curso de Sistemas Operativos  
> 📅 Fecha: Junio 2025  
> 👨‍💻 Autores: Sebastian Amaya Perez, Emmanuel Bustamante Valbuena

---

## 🎯 Objetivo

Implementar y comprender mecanismos de sincronización en programas multi-hilo utilizando:

- 🔐 **Locks (mutexes)**
- 💤 **Condition Variables**
- 🚦 **Semáforos**

Todo esto para garantizar el acceso seguro a recursos compartidos y evitar condiciones de carrera, desbordamientos, bloqueos y más.

---

## 🗂️ Contenido

```bash
📁 lab4-sistemas-operativos/
├─ c/
│   ├─ tsqueue.c
│   ├─ producer_consumer.c
│   └─ dining_philosophers.c
│
└─ go/
    ├─ tsqueue.go
    ├─ producer_consumer.go
    └─ dining_philosophers.go

            
```

---

## ⚙️ Compilación

Cada archivo se compila por separado usando `gcc` con las librerías `pthread` y `rt`:

```bash
gcc -o queue queue.c -lpthread
gcc -o producer_consumer producer_consumer.c -lpthread -lrt
gcc -o dining_philosophers dining_philosophers.c -lpthread
```

---

## 🚀 Ejecución

```bash
./queue
./producer_consumer
./dining_philosophers
```

---

## 🧪 ¿Qué se hizo?

### 🧱 **1. Cola Segura con Mutex y Condition Variable**

- Se creó una estructura `ThreadSafeQueue` para manejar datos compartidos entre hilos productores y consumidores.
- Se usaron:
  - `pthread_mutex_t` para garantizar exclusión mutua.
  - `pthread_cond_t` para suspender consumidores si la cola está vacía.

🔁 Productores y consumidores trabajan de forma segura:

```bash
[Producer 1] Enqueued: 42
[Consumer 2] Dequeued: 42
```

---

### 🧺 **2. Productores y Consumidores con Semáforos**

- Se implementó el problema clásico con:
  - `sem_t full`, `sem_t empty` para controlar acceso al buffer.
  - `pthread_mutex_t` para secciones críticas.

✅ Control de concurrencia en un buffer de tamaño limitado.

```bash
[Producer 1] Produced: 15
[Buffer] Inserted 15
[Consumer 3] Consumed: 15
```

---

### 🍝 **3. Filósofos Comensales sin Deadlock**

- Se simula el ciclo **pensar → tomar tenedores → comer → soltar**.
- Estrategia:
  - Un mutex por tenedor.
  - Semáforo global que limita el número de comensales comiendo simultáneamente (máx 4 de 5).
  
🎯 Resultado: sin interbloqueo y sin inanición.

```bash
[Philosopher 0] Thinking...
[Philosopher 0] Eating 🍝
[Philosopher 0] Finished eating, back to thinking
```

---

## 🖼️ Ejemplos de Salida

```text
=== Queue ===
[Producer 0] Enqueued 23
[Consumer 0] Dequeued 23
[Producer 1] Enqueued 42
[Consumer 1] Dequeued 42

=== Producer-Consumer ===
[Producer 0] Produced 99
[Buffer] Inserted at position 0
[Consumer 1] Removed from position 0
[Consumer 1] Consumed 99

=== Dining Philosophers ===
[Philosopher 2] Thinking...
[Philosopher 2] Eating 🍽️
[Philosopher 2] Thinking again...
```

---

## 💡 Notas Finales

- El código está modularizado y documentado para claridad.
- Se incluyeron delays artificiales para simular procesos reales y probar condiciones de carrera.
- ¡Todo el código es **thread-safe** y se puede escalar fácilmente! 🧵🔧

---

## 📜 Licencia

Este proyecto se entrega como parte del curso de Sistemas Operativos – Universidad de Antioquia.

---

🎓 *¡Aprender sincronización nunca fue tan divertido!*
