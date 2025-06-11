/*
 * producer_consumer.c
 *
 * Solución al problema Productor‐Consumidor con buffer acotado,
 * usando semáforos POSIX (sem_t) y pthread_mutex_t.
 *
 * Compilar: gcc producer_consumer.c -o producer_consumer -pthread -lrt
 * Uso: ./producer_consumer <num_producers> <num_consumers> <buffer_size> <items_per_producer>
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int *buffer;          // Array que actúa como buffer circular
int buffer_size;      // Tamaño máximo del buffer
int in = 0, out = 0;  // Índices para productor (in) y consumidor (out)

sem_t empty_slots;    // Cuenta espacios vacíos
sem_t full_slots;     // Cuenta elementos disponibles
pthread_mutex_t mutex_buffer;

typedef struct {
    int id;
    int items_to_produce;
} ProducerArgs;

typedef struct {
    int id;
    int items_to_consume; // no estrictamente necesario
} ConsumerArgs;

// Función que simula producción de un ítem (valor aleatorio)
int produce_item() {
    return rand() % 1000;
}

// Función que simula consumo de un ítem
void consume_item(int item) {
    // Por simplicidad, solo dormimos un breve tiempo
    usleep(120000);
}

void *producer(void *arg) {
    ProducerArgs *args = (ProducerArgs *)arg;
    for (int i = 0; i < args->items_to_produce; i++) {
        int item = produce_item();
        // Esperar si no hay espacios vacíos
        sem_wait(&empty_slots);
        // Sección crítica para agregar al buffer
        pthread_mutex_lock(&mutex_buffer);
        buffer[in] = item;
        printf("[Producer %d] produjo: %d, lo puso en buffer[%d]\n",
               args->id, item, in);
        in = (in + 1) % buffer_size;
        pthread_mutex_unlock(&mutex_buffer);
        // Señalar que hay un elemento disponible
        sem_post(&full_slots);
        usleep(100000); // Simular algo de tiempo de producción
    }
    return NULL;
}

void *consumer(void *arg) {
    ConsumerArgs *args = (ConsumerArgs *)arg;
    while (1) {
        // Esperar a que haya al menos un elemento
        sem_wait(&full_slots);
        // Sección crítica para remover del buffer
        pthread_mutex_lock(&mutex_buffer);
        int item = buffer[out];
        printf("[Consumer %d] consumió: %d de buffer[%d]\n",
               args->id, item, out);
        out = (out + 1) % buffer_size;
        pthread_mutex_unlock(&mutex_buffer);
        // Señalar que hay un espacio libre
        sem_post(&empty_slots);
        // Simular consumo
        consume_item(item);

        // Condición de salida: si se produce un item especial, o podríamos
        // decidir un contador global. Aquí, salimos cuando se han consumido
        // todos los items posibles (control externo).
        // Para simplificar, no implementamos conteo final; se asumirá que
        // el usuario interrumpe con Ctrl+C o se usa un contador externo.
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr,
                "Uso: %s <num_producers> <num_consumers> <buffer_size> <items_per_producer>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_producers = atoi(argv[1]);
    int num_consumers = atoi(argv[2]);
    buffer_size = atoi(argv[3]);
    int items_per_producer = atoi(argv[4]);

    srand(time(NULL));

    // Reservar buffer dinámicamente
    buffer = (int *)malloc(sizeof(int) * buffer_size);
    if (buffer == NULL) {
        perror("malloc buffer");
        exit(EXIT_FAILURE);
    }

    // Inicializar semáforos
    sem_init(&empty_slots, 0, buffer_size); // inicialmente todos los slots vacíos
    sem_init(&full_slots, 0, 0);            // inicialmente no hay elementos
    pthread_mutex_init(&mutex_buffer, NULL);

    pthread_t producers[num_producers];
    pthread_t consumers[num_consumers];
    ProducerArgs pargs[num_producers];
    ConsumerArgs cargs[num_consumers];

    // Crear hilos consumidores primero (para que esperen si el buffer está vacío)
    for (int i = 0; i < num_consumers; i++) {
        cargs[i].id = i;
        cargs[i].items_to_consume = -1; // no usado directamente
        if (pthread_create(&consumers[i], NULL, consumer, &cargs[i]) != 0) {
            perror("pthread_create consumidor");
            exit(EXIT_FAILURE);
        }
    }

    // Crear hilos productores
    for (int i = 0; i < num_producers; i++) {
        pargs[i].id = i;
        pargs[i].items_to_produce = items_per_producer;
        if (pthread_create(&producers[i], NULL, producer, &pargs[i]) != 0) {
            perror("pthread_create productor");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a que los productores terminen (los consumidores quedan activos)
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }

    // Después de que todos los productores terminaron, podemos terminar
    // el programa. En un caso real, podríamos enviar señales a consumidores
    // para que paren. Aquí, esperaremos unos segundos y luego saldremos.
    sleep(2);

    // Aunque no limpiamos a los consumidores de forma limpia, para propósitos
    // de demostración finalizamos el programa:
    printf("Productores terminaron. Fin del programa.\n");

    // Destruir semáforos y mutex
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&mutex_buffer);
    free(buffer);

    return 0;
}
