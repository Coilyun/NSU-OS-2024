package myhttp

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"net/url"
	"strings"
)

const (
	StatusOK                  = 200
	StatusMovedPermanently    = 301
	StatusFound               = 302
	StatusBadRequest          = 400
	StatusUnauthorized        = 401
	StatusForbidden           = 403
	StatusNotFound            = 404
	StatusInternalServerError = 500
	StatusServiceUnavailable  = 503
)

type Response struct {
	Status     string
	StatusCode int
	Proto      string

	Body io.ReadCloser
}

type bodyReader struct {
	io.Reader
	io.Closer
}

func Get(rawURL string) (*Response, error) {
	parsedURL, err := url.Parse(rawURL)
	if err != nil {
		return nil, fmt.Errorf("invalid URL: %w", err)
	}

	if parsedURL.Scheme != "http" {
		return nil, fmt.Errorf("unsupported scheme: %s (only http is supported)", parsedURL.Scheme)
	}

	host := parsedURL.Hostname()
	port := parsedURL.Port()
	if port == "" {
		port = "80"
	}
	address := net.JoinHostPort(host, port)
	path := parsedURL.RequestURI()

	conn, err := net.Dial("tcp", address)
	if err != nil {
		return nil, fmt.Errorf("failed to connect to %s: %w", address, err)
	}

	request := fmt.Sprintf("GET %s HTTP/1.0\r\n", path)
	request += fmt.Sprintf("Host: %s\r\n", host)
	request += "User-Agent: my-go-client/1.0\r\n"
	request += "Accept: */*\r\n"
	request += "Connection: close\r\n"
	request += "\r\n"

	_, err = conn.Write([]byte(request))
	if err != nil {
		conn.Close()
		return nil, fmt.Errorf("failed to send request: %w", err)
	}

	reader := bufio.NewReader(conn)

	statusLine, err := reader.ReadString('\n')
	if err != nil {
		conn.Close()
		return nil, fmt.Errorf("failed to read status line: %w", err)
	}
	statusLine = strings.TrimSpace(statusLine)

	var proto, status string
	var statusCode int
	_, err = fmt.Sscanf(statusLine, "%s %d %s", &proto, &statusCode, &status)
	if err != nil {
		conn.Close()
		return nil, fmt.Errorf("could not parse status line %q: %w", statusLine, err)
	}

	for {
		line, err := reader.ReadString('\n')
		if err != nil {
			conn.Close()
			return nil, fmt.Errorf("error reading headers: %w", err)
		}
		if line == "\r\n" {
			break
		}
	}

	resp := &Response{
		Status:     status,
		StatusCode: statusCode,
		Proto:      proto,
		Body:       bodyReader{Reader: reader, Closer: conn},
	}

	return resp, nil
}
