/*
 * producer_consumer.go
 *
 * Problema Productor‐Consumidor con buffer acotado en Go.
 * Se implementa un semáforo simple usando canales.
 *
 * Compilar: go build producer_consumer.go
 * Uso: ./producer_consumer <num_producers> <num_consumers> <buffer_size> <items_per_producer>
 */

package main

import (
	"fmt"
	"math/rand"
	"os"
	"strconv"
	"sync"
	"time"
)

// Buffer circular
type CircularBuffer struct {
	data       []int
	size       int
	in, out    int
	lock       sync.Mutex
}

// Inicializar buffer con tamaño n
func NewBuffer(n int) *CircularBuffer {
	return &CircularBuffer{
		data: make([]int, n),
		size: n,
		in:   0,
		out:  0,
	}
}

// Escribir en posición 'in'
func (b *CircularBuffer) Put(item int) {
	b.lock.Lock()
	b.data[b.in] = item
	fmt.Printf("[Producer] puso %d en buffer[%d]\n", item, b.in)
	b.in = (b.in + 1) % b.size
	b.lock.Unlock()
}

// Leer de posición 'out'
func (b *CircularBuffer) Get() int {
	b.lock.Lock()
	item := b.data[b.out]
	fmt.Printf("[Consumer] tomó %d del buffer[%d]\n", item, b.out)
	b.out = (b.out + 1) % b.size
	b.lock.Unlock()
	return item
}

// Semáforo simple basado en canal con a capacidad 'n' para contar recursos
type Semaphore chan struct{}

func NewSemaphore(n int) Semaphore {
	return make(Semaphore, n)
}

// Wait: recibir de canal decrementa contador; si canal está vacío, bloquea hasta haber valor
func (s Semaphore) Wait() {
	<-s
}

// Signal: enviar al canal incrementa contador; si canal está lleno, bloquea o panic (según uso)
func (s Semaphore) Signal() {
	s <- struct{}{}
}

var (
	buffer             *CircularBuffer
	semEmpty, semFull  Semaphore
	itemsPerProducer   int
)

// Simula producción de ítem
func produceItem() int {
	return rand.Intn(1000)
}

// Simula consumo de ítem
func consumeItem(item int) {
	time.Sleep(120 * time.Millisecond)
}

func producer(id int, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < itemsPerProducer; i++ {
		item := produceItem()
		// Esperar espacio vacío
		semEmpty.Wait()
		buffer.Put(item)
		// Indicar que hay elemento disponible
		semFull.Signal()
		time.Sleep(100 * time.Millisecond)
	}
}

func consumer(id int, wg *sync.WaitGroup) {
	defer wg.Done()
	for {
		// Esperar elemento disponible
		semFull.Wait()
		item := buffer.Get()
		// Liberar espacio
		semEmpty.Signal()
		consumeItem(item)
		// En este ejemplo, el consumidor no deja de correr a menos que se aborte
	}
}

func main() {
	if len(os.Args) != 5 {
		fmt.Printf("Uso: %s <num_producers> <num_consumers> <buffer_size> <items_per_producer>\n", os.Args[0])
		os.Exit(1)
	}

	numProducers, _ := strconv.Atoi(os.Args[1])
	numConsumers, _ := strconv.Atoi(os.Args[2])
	bufferSize, _ := strconv.Atoi(os.Args[3])
	itemsPerProducer, _ = strconv.Atoi(os.Args[4])

	rand.Seed(time.Now().UnixNano())

	buffer = NewBuffer(bufferSize)
	semEmpty = NewSemaphore(bufferSize)
	semFull = NewSemaphore(0)
	// Inicializar semEmpty con 'bufferSize' tokens
	for i := 0; i < bufferSize; i++ {
		semEmpty.Signal()
	}

	var wg sync.WaitGroup

	// Iniciar consumidores (quedarán bloqueados en semFull.Wait() hasta que haya elementos)
	for i := 0; i < numConsumers; i++ {
		wg.Add(1)
		go consumer(i, &wg)
	}

	// Iniciar productores
	for i := 0; i < numProducers; i++ {
		wg.Add(1)
		go producer(i, &wg)
	}

	// Esperar a productores
	wg.Wait()

	// Tras terminar productores, los consumidores siguen vivos; se espera unos segundos y se sale
	time.Sleep(2 * time.Second)
	fmt.Println("Productores terminaron. Fin del programa.")
}
