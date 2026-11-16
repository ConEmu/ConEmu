#!/usr/bin/env bash
set -u
cd "$(dirname "$0")/../../.."
source Release/ConEmu/Scripts/ConEmu.NoHooks.bash
PS1='ORIGINAL> '
PROMPT_COMMAND=('printf old' ':')
saved=$(declare -p PROMPT_COMMAND)
conemu_shell_enable
[[ ${PROMPT_COMMAND[0]} == _conemu_prompt ]] || exit 1
printf 'PASS Bash prompt handler prepended to array\n'
false
_conemu_prompt > conpty-bash-osc.bin
status=$?
[[ $status == 1 ]] || exit 2
printf 'PASS Bash prompt preserves last command status\n'
conemu_shell_disable
[[ $(declare -p PROMPT_COMMAND) == "$saved" && $PS1 == 'ORIGINAL> ' ]] || exit 3
printf 'PASS Bash restores array and original prompt\n'
PROMPT_COMMAND='printf string'
conemu_shell_enable
conemu_shell_disable
[[ $PROMPT_COMMAND == 'printf string' ]] || exit 4
printf 'PASS Bash restores string prompt handler\n'
unset PROMPT_COMMAND
conemu_shell_enable
conemu_shell_disable
[[ ! -v PROMPT_COMMAND ]] || exit 5
printf 'PASS Bash restores absent prompt handler\n'
