/*
 * dining_philosophers.go
 *
 * Solución en Go al problema de los Filósofos Comensales.
 * Se emplean sync.Mutex para cada tenedor y un "camarero" implementado
 * con un canal (buffered) que solo permite N-1 filósofos intentando
 * comer simultáneamente.
 *
 * Compilar: go build dining_philosophers.go
 * Uso: ./dining_philosophers <num_philosophers> <num_ciclos_por_filosofo>
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

var numPhilosophers int
var cyclesPerPhilosopher int

// Cada tenedor es un mutex
type Fork struct {
	sync.Mutex
}

// Estructura del filósofo
type Philosopher struct {
	id       int
	left     int
	right    int
	forks    []Fork
	waiterCh chan struct{}
	wg       *sync.WaitGroup
}

// Filósofo piensa
func (p *Philosopher) think() {
	fmt.Printf("[Filósofo %d] Pensando...\n", p.id)
	time.Sleep(time.Duration(200+rand.Intn(200)) * time.Millisecond) // 200-400ms
}

// Filósofo come
func (p *Philosopher) eat(cycle int) {
	fmt.Printf("[Filósofo %d] Comiendo (ciclo %d)...\n", p.id, cycle)
	time.Sleep(time.Duration(250+rand.Intn(250)) * time.Millisecond) // 250-500ms
}

// Ciclo principal del filósofo
func (p *Philosopher) dine() {
	defer p.wg.Done()
	for i := 0; i < cyclesPerPhilosopher; i++ {
		p.think()

		// Solicitar permiso al camarero: si canal con buffer N-1 está vacío, bloquea
		p.waiterCh <- struct{}{}

		// Tomar tenedores en orden (menor índice primero)
		if p.left < p.right {
			p.forks[p.left].Lock()
			p.forks[p.right].Lock()
		} else {
			p.forks[p.right].Lock()
			p.forks[p.left].Lock()
		}

		p.eat(i)

		// Soltar tenedores
		p.forks[p.left].Unlock()
		p.forks[p.right].Unlock()

		// Liberar lugar en el camarero
		<-p.waiterCh
	}

	fmt.Printf("[Filósofo %d] Terminó todos sus ciclos.\n", p.id)
}

func main() {
	if len(os.Args) != 3 {
		fmt.Printf("Uso: %s <num_philosophers> <num_ciclos_por_filosofo>\n", os.Args[0])
		os.Exit(1)
	}
	numPhilosophers, _ = strconv.Atoi(os.Args[1])
	cyclesPerPhilosopher, _ = strconv.Atoi(os.Args[2])

	rand.Seed(time.Now().UnixNano())

	// Crear slice de tenedores
	forks := make([]Fork, numPhilosophers)

	// El canal del camarero tiene buffer de tamaño N-1
	waiterCh := make(chan struct{}, numPhilosophers-1)

	var wg sync.WaitGroup

	// Crear e iniciar filósofos
	for i := 0; i < numPhilosophers; i++ {
		p := &Philosopher{
			id:       i,
			left:     i,
			right:    (i + 1) % numPhilosophers,
			forks:    forks,
			waiterCh: waiterCh,
			wg:       &wg,
		}
		wg.Add(1)
		go p.dine()
	}

	wg.Wait()
	fmt.Println("Todos los filósofos han terminado.")
}
