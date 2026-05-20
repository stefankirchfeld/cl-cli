# cl

A small Claude-powered CLI tool for Linux that acts as a smart shell completion assistant. Type a query in natural language — or mix it with partial commands and typos — and get back ready-to-run shell completions. Also works as a pipe helper for analyzing command output.

## How it works

`cl` has two modes:

**Interactive mode** — launches a minimal TUI. You type your query (natural language, partial command, or a mix), and after a 1-second pause Claude suggests the top 3 completions. Use arrow keys to select and press Enter — the chosen command is printed to stdout.

**Readline injection (Ctrl+K / Ctrl+F)** — when triggered via the shell key bindings, the selected completion is injected directly into your bash readline buffer so you can review and execute it with Enter. This is the primary way to use `cl` for shell completion. When running `cl` as a plain command, output goes to stdout only — copy/paste manually.

**Pipe mode** — reads stdin, combines it with your question, and sends it to Claude. Prints the response to stdout. Useful for analyzing command output, fixing errors, or generating commands from live system state.

## Demo

```
$ # Press Ctrl+K at any bash prompt
> grep for TODO but skip the build directory
  grep -r "TODO" --exclude-dir=build .
  grep -r "TODO" . --exclude-dir={build,.git}
  grep -rn "TODO" --exclude-dir=build .
```

```
$ make 2>&1 | cl -f "what is the error?"
The linker cannot find -ljansson. Install it with: sudo pacman -S jansson
```

## Installation

### Dependencies

- [liblense](https://github.com/stefankirchfeld/liblense) — build system
- `libcurl`
- `jansson`

```bash
sudo pacman -S libcurl jansson   # Arch / Manjaro
```

### Build

```bash
git clone https://github.com/stefankirchfeld/cl-cli
cd cl-cli
llense project register
llense add libcurl
llense add jansson
llense build
```

The binary is written to `build/cl-bin`.

### Setup

1. Copy the binary somewhere on your PATH:

```bash
ln -sf "$PWD/build/cl-bin" ~/.local/bin/cl-bin
```

2. Create your config file:

```bash
mkdir -p ~/.config/cl
cp config.example.ini ~/.config/cl/config.ini
# edit ~/.config/cl/config.ini and set your Anthropic API key
```

3. Source the shell wrapper in your `~/.bashrc`:

```bash
echo 'source ~/src/cl-cli/shell/cl.bash' >> ~/.bashrc
source ~/src/cl-cli/shell/cl.bash
```

## Usage

### Interactive mode

```bash
cl                                  # blank TUI — output printed to stdout
cl find large files modified today  # TUI pre-populated with prompt
```

> **To get readline injection** (result lands in your shell buffer ready to run), use **Ctrl+K** instead of typing `cl` — see Key bindings below.

Or press **Ctrl+K** at any bash prompt — it picks up whatever you've already typed on the line as the initial query, and injects the selected completion back into readline.

Inside the TUI:

| Key | Action |
|-----|--------|
| Type | Enter your query |
| `↑` / `↓` | Browse history (before completions) or select completion |
| `Enter` | Confirm — prints selected completion to stdout (or injects into readline if triggered via Ctrl+K/F) |
| `Ctrl+C` | Cancel |

Completions appear automatically after a 1-second pause in typing.

### Pipe mode

```bash
# analyze output — use -f for free-text answers
git log --oneline -10 | cl -f "what changed recently?"
make 2>&1 | cl -f "what is the build error and how to fix it"
journalctl -xe | tail -30 | cl -f "what is failing?"
cat some_script.sh | cl -f "any bugs?"

# generate a command from live output — no -f, returns a completion
ps aux | cl "kill command for the ssh-agent process"
git log --oneline -10 | cl "command to diff the oldest and newest commit"
du -sh */ | cl "command to remove the largest directory"
```

Note: commands that write errors to stderr need `2>&1` to pipe them:

```bash
git pull 2>&1 | cl -f "fix it"
npm install 2>&1 | cl -f "what went wrong?"
```

### Flags

| Flag | Description |
|------|-------------|
| `-f` | Allow free-text responses. Default (without `-f`) forces completions only — Claude always returns 3 ready-to-run shell commands. |
| `-h`, `--help` | Show help. |

### Key bindings

The shell wrapper (`shell/cl.bash`) registers two readline bindings that provide **direct readline injection** — the selected completion lands in your shell buffer ready to run without copy/paste:

| Binding | Description |
|---------|-------------|
| `Ctrl+K` | Interactive completions mode — picks up current line as initial query |
| `Ctrl+F` | Interactive free-text mode (`-f`) — same, but allows free-text responses |

These are the primary way to use `cl` for shell completion. When running `cl <prompt>` as a plain command, the output is printed to stdout and must be copy/pasted manually.

## Configuration

`~/.config/cl/config.ini`:

```ini
[claude]
api_key = sk-ant-...
model = claude-haiku-4-5-20251001

[context]
commands[] = ls -la
commands[] = git status --short
commands[] = git log --oneline -5
```

**`[claude]`**
- `api_key` — your [Anthropic API key](https://console.anthropic.com/)
- `model` — Claude model ID (default: `claude-haiku-4-5-20251001`)

**`[context]`**
- `commands[]` — shell commands whose output is sent to Claude as context with every request. Useful for giving Claude awareness of your current directory, git state, etc. Run once per invocation.

## History

Queries are saved to `~/.config/cl/history` (max 100 entries, no consecutive duplicates). Browse with arrow up/down in the TUI before completions appear.

## Response protocol

`cl` enforces a strict two-format protocol with Claude:

- **`COMPLETIONS`** — Claude returns exactly 3 ready-to-run shell command lines. Used by default.
- **`FREETEXT`** — Claude returns a free-text answer. Only enabled with `-f`.

This ensures completions are never accidentally executed as multi-line free text.

## License

MIT
