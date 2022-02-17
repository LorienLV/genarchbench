#!/bin/bash

usage() {
    echo "usage: $0 [OPTION]... COMMAND_TO_PROFILE"
    echo "    -e, --events"
    echo "        Events to profile separated by commas or in a range. Valid events are pa1-pa17 [default=pa1-pa17]. e.g:"
    echo "            -e pa1 -> just event pa1"
    echo "            -e pa1,pa2,pa3 -> events pa1, pa2 and pa3"
    echo "            -e pa1-pa5 -> events pa1, pa2, pa3, pa4 and pa5"
    echo "    -l --level"
    echo "        It enables only the regions satisfying \"level\" >= \"third argument level of"
    echo "        measurement region specifying routine\" as a measurement target. [default=0]"
    echo "    -h, --help"
    echo "        Show usage"
}

valid_events="pa1 pa2 pa3 pa4 pa5 pa6 pa7 pa8 pa9 pa10 pa11 pa12 pa13 pa14 pa15 pa16 pa17"

level=0
events="$valid_events"

# exit when any command fails
set -e

while [ "$1" != "" ]; do
    case "$1" in
    -e | --events)
        shift
        events="$1"

        pa_event_regex="pa([1-9]|1[0-7])"

        if [[ "$events" =~ ^$pa_event_regex-$pa_event_regex$ ]]; then
            first="$(echo "$events" | cut -d '-' -f 1)"
            last="$(echo "$events" | cut -d '-' -f 2)"

            match=0 # 0 no matches, 1 first matched, 2 last matched.
            for valid_event in $valid_events; do
                # First matched for the first time.
                if [[ $match -eq 0 && "$valid_event" == "$first" ]]; then
                    events="$first"
                    match=1
                # Filling the array with elements between first and last.
                elif [[ $match -eq 1 && "$valid_event" != $last ]]; then
                    events+=" $valid_event"
                elif [[ $match -eq 1 && "$valid_event" == "$last" ]]; then
                    events+=" $last"
                    match=2
                    break
                fi
            done

            if [[ $match -ne 2 ]]; then
                echo "$first is not < than $last, please, reverse the range."
                usage
                exit 1
            fi
        elif [[ "$events" =~ ^($pa_event_regex,)*$pa_event_regex$ ]]; then
            events="$(echo "$events" | tr ',' ' ')"
        else
            echo "Event string not supported, check the syntax."
            usage
            exit 1
        fi
        ;;
    -l | --level)
        shift
        level="$1"
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    *)
        # The rest of the parameters is the command and its arguments.
        command="$@"
        break
        ;;
    esac
    shift
done

module load fuji

for event in $events; do
    fapp -C -d ./$event -Hevent=$event $command
    fapppx -A -d ./$event -Icpupa,nompi -tcsv -o $event.csv
done
