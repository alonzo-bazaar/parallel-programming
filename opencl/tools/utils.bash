# some bash utilities I might as well put in their own file

LOG_LEVEL_INFO=1
LOG_LEVEL_WARN=2
LOG_LEVEL_ERROR=3
LOG_LEVEL_NONE=10 # above everything else
LOG_LEVEL="${LOG_LEVEL_INFO}"
# current log level sets a treshold
# "I care about this and about everything that's above it"

# LOG_PREFACE will be printed at the start of every logging line
# (with the functions we have here anyhow)
# can be set it before running any of the below logging functions to,
# for instance, make it clear from whom are the logs coming, having like
# LOG_PREFACE="[$0] "
# or
# LOG_PREFACE="==$$== " # to look like valgrind
# default value is just have nothing
LOG_PREFACE=""

# text colouring utilities that honour the NO_COLOR environment variable
# (and NO_COLOUR as well because funny)
# ansi escapes taken from
# https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b
color() {
    printf '\e[%dm' "$1"
}
with_color() {
    local text_color="$1"
    local text="$2"
    if [[ -v NO_COLOR || -v NO_COLOUR ]]; then
        echo "${text}"
    else
        color "$text_color"
        echo "${text}"
        color 0 # 0 is the reset color
    fi
}

# coloring functions
red() {
    with_color 31 "$*"
}
green() {
    with_color 32 "$*"
}
yellow() {
    with_color 33 "$*"
}

# colored logging utilities:
# the colors they're gonna use
# here with their default values, but left global for user settability
ERROR_COLOR=31  # ansi red
FATAL_COLOR=31  # ansi red
INFO_COLOR=32   # ansi green
WARN_COLOR=33   # ansi yellow
DEBUG_COLOR=33  # ansi yellow

# the utilities in question
info() {
    if (( LOG_LEVEL_INFO >= LOG_LEVEL )); then
        with_color "${INFO_COLOR}" "${LOG_PREFACE}[INFO] $*"
    fi
}
warn() {
    if (( LOG_LEVEL_WARN >= LOG_LEVEL )); then
        with_color "${WARN_COLOR}" "${LOG_PREFACE}[WARNING] $*" >&2
    fi
}
error() {
    if (( LOG_LEVEL_ERROR >= LOG_LEVEL )); then
        with_color "${ERROR_COLOR}" "${LOG_PREFACE}[ERROR] $*" >&2
    fi
}
debug() {
    if [[ -v $DEBUG ]]; then
        with_color "${DEBUG_COLOR}" "${LOG_PREFACE}[DEBUG] $*" >&2
    fi
}
fatal() {
    with_color "${FATAL_COLOR}" "${LOG_PREFACE}[FATAL ERROR] $*" >&2
    exit 1
}

# https://oneuptime.com/blog/post/2026-02-08-how-to-interpret-all-docker-container-exit-codes/view
# https://man.openbsd.org/sysexits
log_exit_code() {
    local code="$1"
    local prefix="[exit code ${code}]: " 
    case "${code}" in
        # the classics
        0)   info "${prefix}successfully exit" ;;
        1)   error "${prefix}generic error happened" ;;
        2)   error "${prefix}incorrect usage" ;;

        # the bsd ones
        64)  error "${prefix}incorrect usage" ;;
        65)  error "${prefix}data error" ;;
        66)  error "${prefix}input not provided or not readable" ;;
        67)  error "${prefix}user does not exist" ;;
        68)  error "${prefix}host does not exist" ;;
        69)  error "${prefix}unavailable service or resource" ;;
        70)  error "${prefix}internal software error (unrelated to os)" ;;
        71)  error "${prefix}os side error" ;;
        72)  error "${prefix}some system file non existant or not readable" ;;
        73)  error "${prefix}unable to create output file" ;;
        74)  error "${prefix}error occured during io to some file" ;;
        75)  warn  "${prefix}temporary failure, try again later" ;;
        76)  error "${prefix}protocol error, received implausible data" ;;
        77)  error "${prefix}insufficient permissions to perform operation" ;;
        78)  error "${prefix}something was unconfigured or misconfigured" ;;

        # some shit that was in the docker article
        125) error "${prefix}(if docker) daemon error" ;;
        126) error "${prefix}something was not executable" ;;
        127) error "${prefix}command not found" ;;
        128) error "${prefix}idfk" ;;
    esac

    # death by signal
    if (( code > 128 )); then
        local ending_signal=$((code - 128))
        local sig_names=(
            ""
            "SIGHUP: hangup, terminal disconnected"
            "SIGINT: interrupted, most likey ctrl-c"
            "SIGQUIT: quit (core dumped)"
            "SIGILL: tried executing an illegal instruction"
            "SIGTRAP: trace/breakpoint trap"
            "SIGABRT: call to abort()"
            "SIGBUS: bus error"
            "SIGFPE: floating point exception"
            "SIGKILL: was killed"
            "SIGUSR1: user defined signal 1"
            "SIGSEGV: segmentation fault (core dumped)"
            "SIGUSR2: user defined signal 2"
            "SIGPIPE: broken pipe"
            "SIGALRM: alalrm clock"
            "SIGTERM: was gracefully terminated"
        )
        local msg="${prefix}killed by"
        msg+="signal number ${ending_signal} (${sig_names[ending_signal]})"
        if ((ending_signal==2 || ending_signal==15)); then
            info "$msg"
        else
            error "$msg"
        fi
    fi
}

