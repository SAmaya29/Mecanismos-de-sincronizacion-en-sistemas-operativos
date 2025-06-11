/*
 * tsqueue.c
 *
 * Implementación de una cola thread-safe usando pthread_mutex_t y pthread_cond_t.
 * Múltiples hilos productores y consumidores pueden encolar y desencolar
 * sin condiciones de carrera. Si la cola está vacía, los consumidores esperan.
 *
 * Compilar: gcc tsqueue.c -o tsqueue -pthread
 * Uso: ./tsqueue <num_producers> <num_consumers> <items_per_producer>
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} ThreadSafeQueue;

// Inicializa la cola
void queue_init(ThreadSafeQueue *q) {
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

// Encola un elemento al final
void enqueue(ThreadSafeQueue *q, int item) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_node->value = item;
    new_node->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->tail == NULL) {
        // Si está vacía, head y tail apuntan al mismo nodo
        q->head = q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
    // Señalizamos a cualquier consumidor que esté esperando
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Desencola un elemento; si está vacía, espera
int dequeue(ThreadSafeQueue *q) {
    pthread_mutex_lock(&q->lock);
    while (q->head == NULL) {
        // Esperar hasta que no esté vacía
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    Node *to_free = q->head;
    int result = to_free->value;
    q->head = q->head->next;
    if (q->head == NULL) {
        // Si quedó vacía, tail también a NULL
        q->tail = NULL;
    }
    free(to_free);
    pthread_mutex_unlock(&q->lock);
    return result;
}

// Variables globales para pasar parámetros a hilos
typedef struct {
    ThreadSafeQueue *queue;
    int producer_id;
    int items_to_produce;
} ProducerArgs;

typedef struct {
    ThreadSafeQueue *queue;
    int consumer_id;
    int total_items; // solo para saber cuándo detenerse
    int *consumed_count; // contador compartido
    pthread_mutex_t *count_lock;
} ConsumerArgs;

// Función de productor: encola items_to_produce elementos
void *producer_thread(void *arg) {
    ProducerArgs *args = (ProducerArgs *)arg;
    for (int i = 0; i < args->items_to_produce; i++) {
        int item = args->producer_id * 1000 + i; // valor único según productor e índice
        printf("[Producer %d] Enqueuing item %d\n", args->producer_id, item);
        enqueue(args->queue, item);
        // Opcional: dormir un poco para simular trabajo
        usleep(100000); // 100 ms
    }
    return NULL;
}

// Función de consumidor: desencola hasta que se consuman total_items
void *consumer_thread(void *arg) {
    ConsumerArgs *args = (ConsumerArgs *)arg;
    while (1) {
        // Chequear si ya consumimos todos los items esperados
        pthread_mutex_lock(args->count_lock);
        if (*(args->consumed_count) >= args->total_items) {
            pthread_mutex_unlock(args->count_lock);
            break;
        }
        pthread_mutex_unlock(args->count_lock);

        int item = dequeue(args->queue);
        pthread_mutex_lock(args->count_lock);
        (*(args->consumed_count))++;
        int local_count = *(args->consumed_count);
        pthread_mutex_unlock(args->count_lock);

        printf("[Consumer %d] Dequeued item %d (consumido #%d)\n",
               args->consumer_id, item, local_count);
        // Simular consumo
        usleep(150000); // 150 ms
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <num_producers> <num_consumers> <items_per_producer>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int num_producers = atoi(argv[1]);
    int num_consumers = atoi(argv[2]);
    int items_per_producer = atoi(argv[3]);

    ThreadSafeQueue queue;
    queue_init(&queue);

    pthread_t producers[num_producers];
    pthread_t consumers[num_consumers];

    ProducerArgs pargs[num_producers];
    ConsumerArgs cargs[num_consumers];

    // Contador total de elementos que deben consumirse
    int total_items = num_producers * items_per_producer;
    int consumed_count = 0;
    pthread_mutex_t count_lock;
    pthread_mutex_init(&count_lock, NULL);

    // Crear hilos productores
    for (int i = 0; i < num_producers; i++) {
        pargs[i].queue = &queue;
        pargs[i].producer_id = i;
        pargs[i].items_to_produce = items_per_producer;
        if (pthread_create(&producers[i], NULL, producer_thread, &pargs[i]) != 0) {
            perror("pthread_create productor");
            exit(EXIT_FAILURE);
        }
    }

    // Crear hilos consumidores
    for (int i = 0; i < num_consumers; i++) {
        cargs[i].queue = &queue;
        cargs[i].consumer_id = i;
        cargs[i].total_items = total_items;
        cargs[i].consumed_count = &consumed_count;
        cargs[i].count_lock = &count_lock;
        if (pthread_create(&consumers[i], NULL, consumer_thread, &cargs[i]) != 0) {
            perror("pthread_create consumidor");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a productores
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    // Después de que todos los productores terminaron, es posible que
    // queden consumidores esperando. Despertarlos para que chequeen el conteo.
    pthread_cond_broadcast(&queue.not_empty);

    // Esperar a consumidores
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    // Destruir mutexes y cond
    pthread_mutex_destroy(&queue.lock);
    pthread_cond_destroy(&queue.not_empty);
    pthread_mutex_destroy(&count_lock);

    printf("Todos los productores y consumidores han finalizado.\n");
    return 0;
}
