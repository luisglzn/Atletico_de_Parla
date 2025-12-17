#!/bin/bash

cd player/build && make && cd ../..

BUILD_DIR="Player/build"

for i in {0..10}
do
    # Construir el parámetro extra solo en la primera iteración
    extra=""
    if [ "$i" -eq 0 ]; then
	cd "BUILD_DIR"
        xterm -hold -e "./Player AtleticoDeParla 807$i goalie" &
    fi

    echo "Lanzando jugador $i"
    (
        cd "$BUILD_DIR"
        xterm -hold -e "./Player AtleticoDeParla 807$i" &
    ) &
    sleep 0.3
done

wait

