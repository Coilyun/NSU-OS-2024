package main

import (
	"fmt"
	"log"
	"sync"
	"sync/atomic"
	"time"
)

type CountingSemaphore struct {
	mu    sync.Mutex
	cond  *sync.Cond
	count int
}

func NewCountingSemaphore() *CountingSemaphore {
	s := &CountingSemaphore{}
	s.cond = sync.NewCond(&s.mu)
	return s
}

func (s *CountingSemaphore) Acquire() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	for s.count == 0 {
		s.cond.Wait()
	}

	s.count--
	return nil
}

func (s *CountingSemaphore) Release() {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.count++
	s.cond.Signal()
}

func (s *CountingSemaphore) Count() int {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.count
}

// Конфигурация производственной линии
const (
	durationA = 1 * time.Second
	durationB = 2 * time.Second
	durationC = 3 * time.Second
)

func init() {
	log.SetFlags(0) // Убирает префиксы времени/даты из логов
}

// producer производит детали, имитируя задержку тикером
func producer(
	partName string,
	productionTime time.Duration,
	detailSem *CountingSemaphore,
) {
	ticker := time.NewTicker(productionTime)
	defer ticker.Stop()

	for {
		<-ticker.C
		detailSem.Release()
		log.Printf("[%s Producer] -> Деталь отправлена.", partName)
	}
}

// moduleAssembler собирает модуль из деталей A и B
func moduleAssembler(
	moduleSem *CountingSemaphore,
) {
	assemblerName := fmt.Sprintf("Сборщик Модулей")

	detailASem := NewCountingSemaphore()
	detailBSem := NewCountingSemaphore()

	go producer("Деталь А", durationA, detailASem)
	go producer("Деталь B", durationB, detailBSem)

	for {
		detailASem.Acquire()
		detailBSem.Acquire()
		moduleSem.Release()
		log.Printf("[%s] -> Модуль отправлен", assemblerName)
	}
}

// Собирает винтик из Модуля и детали C.
func widgetAssembler(
	widgetCounter *int64,
) {
	assemblerName := fmt.Sprintf("Сборщик Винтиков")

	moduleCSem := NewCountingSemaphore()
	detailSem := NewCountingSemaphore()

	go producer("Деталь С", durationC, moduleCSem)
	go moduleAssembler(detailSem)

	for {
		moduleCSem.Acquire()
		detailSem.Acquire()
		newCount := atomic.AddInt64(widgetCounter, 1)
		log.Printf("[%s] ===> Собран Винтик #%d", assemblerName, newCount)
	}
}

func main() {
	log.Printf("[Main] Запуск производственной линии")

	var wg sync.WaitGroup
	var widgetCounter int64

	wg.Add(1)
	go func() {
		defer wg.Done()
		widgetAssembler(&widgetCounter)
	}()

	wg.Wait()
}
