# Source explicitly in Bash. No startup files are modified.
# source /path/to/ConEmu.NoHooks.bash; conemu_shell_enable
conemu_shell_enable() {
    [[ $- == *i* ]] || return 1
    [[ ${_conemu_shell_enabled:-0} == 1 ]] && return 0
    _conemu_shell_enabled=1
    _conemu_saved_ps1=$PS1
    _conemu_saved_prompt_command=$(declare -p PROMPT_COMMAND 2>/dev/null) || _conemu_saved_prompt_command=
    if declare -p PROMPT_COMMAND 2>/dev/null | command grep -q 'declare -a'; then
        PROMPT_COMMAND=(_conemu_prompt "${PROMPT_COMMAND[@]}")
    else
        PROMPT_COMMAND="_conemu_prompt${PROMPT_COMMAND:+; $PROMPT_COMMAND}"
    fi
    PS1=$PS1'\[\e]133;B\e\\\]'
}
_conemu_prompt() {
    local status=$?
    printf '\e]133;D;%d\e\\\e]9;4;0\e\\\e]133;A\e\\' "$status"
    # Percent-encode bytes outside the URI path-safe set, including ESC and BEL.
    local path=$PWD host= uri= char byte i LC_ALL=C
    if command -v cygpath >/dev/null 2>&1; then
        path=/$(cygpath -am "$PWD")
    elif [[ -n ${WSL_DISTRO_NAME:-} ]]; then
        host=wsl.localhost
        path=/$WSL_DISTRO_NAME$PWD
    fi
    for ((i=0; i<${#path}; i++)); do
        char=${path:i:1}
        case $char in [a-zA-Z0-9/:._~-]) uri+=$char;; *) printf -v byte '%%%02X' "'$char"; uri+=$byte;; esac
    done
    printf '\e]7;file://%s%s\e\\' "$host" "$uri"
    return "$status"
}
conemu_shell_disable() {
    [[ ${_conemu_shell_enabled:-0} == 1 ]] || return 0
    PS1=$_conemu_saved_ps1
    unset PROMPT_COMMAND
    [[ -z $_conemu_saved_prompt_command ]] || eval "${_conemu_saved_prompt_command/declare /declare -g }"
    unset _conemu_shell_enabled _conemu_saved_prompt_command _conemu_saved_ps1
}
