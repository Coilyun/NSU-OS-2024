package pager

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"sem2-lab30/terminal"
)

var (
	ErrInterrupted  = errors.New("interrupted by user")
	ErrTerminalRead = errors.New("terminal read failed")
)

const (
	defaultLinesPerPage = 25
	keySpace            = ' '
	keySmallQ           = 'q'
	keyBigQ             = 'Q'
	keyCtrlC            = 3
	promptMessage       = "--- Press space to scroll, 'q' to quit ---"
	clearLine           = "\r                                                \r"
)

type Pager struct {
	linesPrinted int
	linesPerPage int
	isEnabled    bool
	stdinFd      int
}

func New() *Pager {
	stdinFd := int(os.Stdin.Fd())
	stdoutFd := int(os.Stdout.Fd())
	isEnabled := terminal.IsTerminal(stdinFd) && terminal.IsTerminal(stdoutFd)
	return &Pager{
		linesPerPage: defaultLinesPerPage,
		isEnabled:    isEnabled,
		stdinFd:      stdinFd,
	}
}

func (p *Pager) Print(chunk []byte) error {
	fmt.Printf("%s", chunk)

	p.linesPrinted++

	if p.isEnabled && p.linesPrinted >= p.linesPerPage {
		if err := p.waitForInput(); err != nil {
			return err
		}
		p.linesPrinted = 0
	}

	return nil
}

func (p *Pager) waitForInput() error {
	fmt.Print(promptMessage)
	defer fmt.Print(clearLine)

	initialState, err := terminal.MakeRaw(p.stdinFd)
	if err != nil {
		fmt.Fprintf(os.Stderr, "\nwarning: cannot set raw mode: %v\n--- Press Enter to continue ---\n", err)
		bufio.NewReader(os.Stdin).ReadBytes('\n')
		return nil
	}
	defer terminal.Restore(p.stdinFd, initialState)

	keyBuf := make([]byte, 1)
	for {
		n, err := os.Stdin.Read(keyBuf)
		if err != nil {
			return fmt.Errorf("%w: %w", ErrTerminalRead, err)
		}
		if n == 1 {
			switch keyBuf[0] {
			case keySpace:
				return nil
			case keySmallQ, keyBigQ, keyCtrlC:
				return ErrInterrupted
			}
		}
	}
}
