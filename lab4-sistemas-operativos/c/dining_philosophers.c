/*
 * dining_philosophers.c
 *
 * Solución al problema de los Filósofos Comensales en C usando pthreads.
 * Se asegura que no haya deadlock usando un semáforo “camarero” que solo
 * permita a N-1 filósofos intentar tomar tenedores simultáneamente.
 *
 * Compilar: gcc dining_philosophers.c -o dining_philosophers -pthread -lrt
 * Uso: ./dining_philosophers <num_philosophers> <num_ciclos_por_filosofo>
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int num_philosophers;
int cycles_per_philosopher;

// Cada tenedor es un mutex
pthread_mutex_t *forks;

// Semáforo camarero (permite hasta num_philosophers-1 a la vez)
sem_t waiter;

typedef struct {
    int id;
} PhilosopherArgs;

// Simula pensar
void think(int id) {
    printf("[Filósofo %d] Pensando...\n", id);
    usleep(200000 + (rand() % 200000)); // 200-400 ms
}

// Simula comer
void eat(int id, int cycle) {
    printf("[Filósofo %d] Comiendo (ciclo %d)...\n", id, cycle);
    usleep(250000 + (rand() % 250000)); // 250-500 ms
}

void *philosopher(void *arg) {
    PhilosopherArgs *args = (PhilosopherArgs *)arg;
    int id = args->id;
    int left = id;                     // índice del tenedor izquierdo
    int right = (id + 1) % num_philosophers; // índice del tenedor derecho

    for (int i = 0; i < cycles_per_philosopher; i++) {
        think(id);

        // Solicitar permiso al camarero (semáforo). Solo num_philosophers-1 pueden tomar en conjunto.
        sem_wait(&waiter);

        // Tomar tenedores: primero el de menor índice (para mantener orden y evitar deadlock)
        if (left < right) {
            pthread_mutex_lock(&forks[left]);
            pthread_mutex_lock(&forks[right]);
        } else {
            pthread_mutex_lock(&forks[right]);
            pthread_mutex_lock(&forks[left]);
        }

        // Ahora come
        eat(id, i);

        // Dejar tenedores
        pthread_mutex_unlock(&forks[left]);
        pthread_mutex_unlock(&forks[right]);

        // Liberar espacio en el camarero
        sem_post(&waiter);
    }

    printf("[Filósofo %d] Terminó todos sus ciclos.\n", id);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,
                "Uso: %s <num_philosophers> <num_ciclos_por_filosofo>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    num_philosophers = atoi(argv[1]);
    cycles_per_philosopher = atoi(argv[2]);

    srand(time(NULL));

    forks = malloc(sizeof(pthread_mutex_t) * num_philosophers);
    for (int i = 0; i < num_philosophers; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }

    // Inicializar semáforo camarero a num_philosophers-1
    sem_init(&waiter, 0, num_philosophers - 1);

    pthread_t phils[num_philosophers];
    PhilosopherArgs args[num_philosophers];

    // Crear hilos filósofos
    for (int i = 0; i < num_philosophers; i++) {
        args[i].id = i;
        if (pthread_create(&phils[i], NULL, philosopher, &args[i]) != 0) {
            perror("pthread_create filósofo");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a todos los filósofos
    for (int i = 0; i < num_philosophers; i++) {
        pthread_join(phils[i], NULL);
    }

    // Destruir mutexes y semáforo
    for (int i = 0; i < num_philosophers; i++) {
        pthread_mutex_destroy(&forks[i]);
    }
    free(forks);
    sem_destroy(&waiter);

    printf("Todos los filósofos han terminado.\n");
    return 0;
}
