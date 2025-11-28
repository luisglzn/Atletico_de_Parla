#!/bin/bash


BUILD_DIR="$HOME/tecnobit/formacion/Player/player_robocup/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: El directorio de build no existe en: $BUILD_DIR"
    exit 1
fi

echo "Lanzando 11 jugadores..."

for i in {0..10}
do

  CMD_TO_RUN="cd $BUILD_DIR && ./player_robocup AtleticoDeParla 555$i"
  
  gnome-terminal -- bash -c "$CMD_TO_RUN" &
  
  sleep 1
done

echo "Lanzamiento completado."
