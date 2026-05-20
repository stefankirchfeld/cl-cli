# Source this in ~/.zshrc:
#   source ~/src/cl-cli/shell/cl.zsh
#
# Usage:
#   Ctrl+K                      — interactive completion (picks up current line as initial prompt)
#   Ctrl+F                      — interactive free text mode (-f flag, same)
#   cat file | cl <question>    — pipe mode, completions only
#   cat file | cl -f <question> — pipe mode, allow free text answer
#   cl <question>               — print selected completion to stdout (no readline injection)

_cl_strip_cmd() {
    local line="$1"
    line="${line#cl -f }"
    line="${line#cl -f}"
    line="${line#cl }"
    line="${line#cl}"
    echo "$line"
}

_cl_run() {
    local initial tmpfile rc
    initial=$(_cl_strip_cmd "$BUFFER")
    tmpfile=$(mktemp)
    BUFFER=""
    CURSOR=0
    # Run cl-bin with terminal fully attached; result written to tmpfile
    cl-bin "$@" ${initial:+"$initial"} >"$tmpfile" </dev/tty 2>/dev/tty
    rc=$?
    if [[ $rc -eq 0 ]]; then
        local result
        result=$(<"$tmpfile")
        if [[ -n "$result" ]]; then
            BUFFER="$result"
            CURSOR=${#result}
        fi
    fi
    rm -f "$tmpfile"
    zle reset-prompt
}

_cl_interactive_with_line()         { _cl_run }
_cl_interactive_freetext_with_line() { _cl_run -f }

zle -N _cl_interactive_with_line
zle -N _cl_interactive_freetext_with_line

bindkey '^K' _cl_interactive_with_line
bindkey '^F' _cl_interactive_freetext_with_line

cl() {
    cl-bin "$@"
}
