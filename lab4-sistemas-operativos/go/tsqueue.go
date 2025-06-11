/*
 * tsqueue.go
 *
 * Implementación en Go de una cola thread-safe (Múltiples productores y consumidores).
 * Se usa sync.Mutex y sync.Cond para proteger y coordinar acceso.
 *
 * Compilar: go build tsqueue.go
 * Uso: ./tsqueue <num_producers> <num_consumers> <items_per_producer>
 */

package main

import (
	"fmt"
	"os"
	"strconv"
	"sync"
	"time"
)

// ThreadSafeQueue implementada como slice dinámico
type ThreadSafeQueue struct {
	items []int
	lock  sync.Mutex
	cond  *sync.Cond
}

// Crea una nueva cola vacía
func NewQueue() *ThreadSafeQueue {
	q := &ThreadSafeQueue{
		items: make([]int, 0),
	}
	q.cond = sync.NewCond(&q.lock)
	return q
}

// Encola un elemento
func (q *ThreadSafeQueue) Enqueue(item int) {
	q.lock.Lock()
	q.items = append(q.items, item)
	// Señalizar que ya no está vacía
	q.cond.Signal()
	q.lock.Unlock()
}

// Desencola un elemento; si está vacía, espera
func (q *ThreadSafeQueue) Dequeue() int {
	q.lock.Lock()
	for len(q.items) == 0 {
		q.cond.Wait()
	}
	item := q.items[0]
	q.items = q.items[1:]
	q.lock.Unlock()
	return item
}

var totalConsumed int
var totalToConsume int
var countLock sync.Mutex

func producer(queue *ThreadSafeQueue, id int, itemsToProduce int, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < itemsToProduce; i++ {
		item := id*1000 + i
		fmt.Printf("[Producer %d] Enqueuing item %d\n", id, item)
		queue.Enqueue(item)
		time.Sleep(100 * time.Millisecond)
	}
}

func consumer(queue *ThreadSafeQueue, id int, wg *sync.WaitGroup) {
	defer wg.Done()
	for {
		// Verificar si ya consumimos todo
		countLock.Lock()
		if totalConsumed >= totalToConsume {
			countLock.Unlock()
			return
		}
		countLock.Unlock()

		item := queue.Dequeue()
		countLock.Lock()
		totalConsumed++
		cur := totalConsumed
		countLock.Unlock()
		fmt.Printf("[Consumer %d] Dequeued item %d (consumido #%d)\n", id, item, cur)
		time.Sleep(150 * time.Millisecond)
	}
}

func main() {
	if len(os.Args) != 4 {
		fmt.Printf("Uso: %s <num_producers> <num_consumers> <items_per_producer>\n", os.Args[0])
		os.Exit(1)
	}
	numProducers, _ := strconv.Atoi(os.Args[1])
	numConsumers, _ := strconv.Atoi(os.Args[2])
	itemsPerProducer, _ := strconv.Atoi(os.Args[3])

	queue := NewQueue()
	totalToConsume = numProducers * itemsPerProducer

	var wg sync.WaitGroup

	// Iniciar productores
	for i := 0; i < numProducers; i++ {
		wg.Add(1)
		go producer(queue, i, itemsPerProducer, &wg)
	}

	// Iniciar consumidores
	for i := 0; i < numConsumers; i++ {
		wg.Add(1)
		go consumer(queue, i, &wg)
	}

	// Esperar a que todos terminen
	wg.Wait()
	fmt.Println("Todos los productores y consumidores han finalizado.")
}
