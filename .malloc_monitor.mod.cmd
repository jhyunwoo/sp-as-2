savedcmd_malloc_monitor.mod := printf '%s\n'   malloc_monitor.o | awk '!x[$$0]++ { print("./"$$0) }' > malloc_monitor.mod
