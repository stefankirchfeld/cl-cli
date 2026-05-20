# Source this in ~/.bashrc:
#   source ~/src/cl-cli/shell/cl.bash
#
# Usage:
#   Ctrl+K                      — interactive completion (picks up current line as initial prompt)
#   Ctrl+F                      — interactive free text mode (-f flag, same)
#   cat file | cl <question>    — pipe mode, completions only
#   cat file | cl -f <question> — pipe mode, allow free text answer
#   cl <question>               — print selected completion to stdout (no readline injection)

_cl_strip_cmd() {
    # strip leading "cl" or "cl -f" from the readline buffer
    local line="$1"
    line="${line#cl -f }"
    line="${line#cl -f}"
    line="${line#cl }"
    line="${line#cl}"
    echo "$line"
}

_cl_interactive_with_line() {
    local initial
    initial=$(_cl_strip_cmd "$READLINE_LINE")
    READLINE_LINE=""
    READLINE_POINT=0
    local result
    result=$(cl-bin ${initial:+"$initial"} 2>/dev/tty)
    if [[ $? -eq 0 && -n "$result" ]]; then
        READLINE_LINE="$result"
        READLINE_POINT=${#result}
    fi
}

_cl_interactive_freetext_with_line() {
    local initial
    initial=$(_cl_strip_cmd "$READLINE_LINE")
    READLINE_LINE=""
    READLINE_POINT=0
    local result
    result=$(cl-bin -f ${initial:+"$initial"} 2>/dev/tty)
    if [[ $? -eq 0 && -n "$result" ]]; then
        READLINE_LINE="$result"
        READLINE_POINT=${#result}
    fi
}

cl() {
    cl-bin "$@"
}

bind -x '"\C-k": _cl_interactive_with_line'
bind -x '"\C-f": _cl_interactive_freetext_with_line'
