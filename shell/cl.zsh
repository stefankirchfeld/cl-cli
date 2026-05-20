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

_cl_interactive_with_line() {
    local initial result
    initial=$(_cl_strip_cmd "$BUFFER")
    BUFFER=""
    CURSOR=0
    result=$(cl-bin ${initial:+"$initial"} 2>/dev/tty)
    if [[ $? -eq 0 && -n "$result" ]]; then
        BUFFER="$result"
        CURSOR=${#result}
    fi
    zle reset-prompt
}

_cl_interactive_freetext_with_line() {
    local initial result
    initial=$(_cl_strip_cmd "$BUFFER")
    BUFFER=""
    CURSOR=0
    result=$(cl-bin -f ${initial:+"$initial"} 2>/dev/tty)
    if [[ $? -eq 0 && -n "$result" ]]; then
        BUFFER="$result"
        CURSOR=${#result}
    fi
    zle reset-prompt
}

zle -N _cl_interactive_with_line
zle -N _cl_interactive_freetext_with_line

bindkey '^K' _cl_interactive_with_line
bindkey '^F' _cl_interactive_freetext_with_line

cl() {
    cl-bin "$@"
}
