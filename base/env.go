package blockchain

import (
	"os"
	"path/filepath"
)

// HomeDirFromEnvironment returns the directory to use
// for reading config and storing variable data.
// or, if that is empty, $HOME/.chaincore.
func HomeDirFromEnvironment() string {
	if s := os.Getenv("BTMINER_HOME"); s != "" {
		return s
	}
	return filepath.Join(os.Getenv("HOME"), ".btminer")
}
