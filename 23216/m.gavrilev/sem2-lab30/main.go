package main

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"sem2-lab30/myhttp"
	pager "sem2-lab30/paginator"
	"sync"
)

const (
	networkBufferSize  = 4096
	errorResponseLimit = 512
	dataChannelBuffer  = 100
	scannerMaxCapacity = 1024 * 1024
)

func main() {
	log.SetPrefix("error: ")
	log.SetFlags(0)

	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "usage: %s <URL>\n", os.Args[0])
		os.Exit(1)
	}
	url := os.Args[1]

	fmt.Fprintf(os.Stderr, "[Fetching %s ...]\n", url)
	resp, err := myhttp.Get(url)
	if err != nil {
		log.Fatalf("failed to fetch URL %s: %v", url, err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != myhttp.StatusOK {
		bodyBytes, _ := ioutil.ReadAll(io.LimitReader(resp.Body, errorResponseLimit))
		log.Fatalf("received non-200 status code: %d %s\nResponse body (partial): %s",
			resp.StatusCode, resp.Status, string(bodyBytes))
	}

	dataChan := make(chan []byte)
	errChan := make(chan error, 1)

	var wg sync.WaitGroup
	wg.Add(2)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	go func() {
		defer wg.Done()
		networkReader(ctx, resp.Body, dataChan)
	}()
	go func() {
		defer wg.Done()
		userInteractor(ctx, errChan, dataChan)
	}()

	if finalErr := <-errChan; finalErr != nil {
		cancel()
		if errors.Is(finalErr, pager.ErrInterrupted) {
			fmt.Fprintf(os.Stderr, "\n[Interrupted by user]\n")
		} else {
			log.Printf("critical error during interaction: %v", finalErr)
		}
	}

	wg.Wait()
	fmt.Fprintf(os.Stderr, "\n[Done]\n")
}

func networkReader(ctx context.Context, body io.ReadCloser, dataChan chan<- []byte) {
	defer close(dataChan)

	buffer := make([]byte, networkBufferSize)
	for {
		n, err := body.Read(buffer)
		if n > 0 {
			dataCopy := make([]byte, n)
			copy(dataCopy, buffer[:n])
			select {
			case dataChan <- dataCopy:
			case <-ctx.Done():
				return
			}
		}
		if err != nil {
			if err != io.EOF {
				log.Printf("reading response body error: %v", err)
			}
			break
		}
	}
}

func userInteractor(ctx context.Context, errChan chan<- error, dataChan <-chan []byte) {
	defer close(errChan)
	p := pager.New()
	lastToken := []byte{}

	for dataChunk := range dataChan {
		select {
		case <-ctx.Done():
			return
		default:
		}

		chunkToProcess := append(lastToken, dataChunk...)
		tokens := bytes.SplitAfter(chunkToProcess, []byte("\n"))

		for i := 0; i < len(tokens)-1; i++ {
			if len(tokens[i]) > 0 {
				if err := p.Print(tokens[i]); err != nil {
					errChan <- err
					return
				}
			}
		}

		lastToken = tokens[len(tokens)-1]
	}

	if len(lastToken) > 0 {
		if err := p.Print(lastToken); err != nil {
			errChan <- err
		}
	}
}
