#!/bin/bash

run_with_timeout() {
    local timeout_seconds=$1
    shift
    local start_time=$(date +%s)

    setsid -- "$@" &
    local pid=$!
    local pgid=$pid

    while kill -0 "$pid" 2>/dev/null; do
        local now elapsed
        now=$(date +%s)
        elapsed=$((now - start_time))

        if [ "$elapsed" -ge "$timeout_seconds" ]; then
            echo "⏱ over $timeout_seconds seconds, send SIGINT to process group $pgid"
            kill -INT -"${pgid}" 2>/dev/null || true

            local waited=0
            while kill -0 "$pid" 2>/dev/null && [ $waited -lt 120 ]; do
                sleep 1
                waited=$((waited+1))
            done

            if kill -0 "$pid" 2>/dev/null; then
                echo "⚠️ Send SIGTERM to process group $pgid"
                kill -TERM -"${pgid}" 2>/dev/null || true
            fi
            local waited2=0
            while kill -0 "$pid" 2>/dev/null && [ $waited2 -lt 2 ]; do
                sleep 1
                waited2=$((waited2+1))
            done

            if kill -0 "$pid" 2>/dev/null; then
                echo "💥 Still alive, send SIGKILL to process group $pgid"
                kill -KILL -"${pgid}" 2>/dev/null || true
            fi
            break
        fi

        sleep 1
    done

    # wait "$pid" 2>/dev/null
    wait $pid
}