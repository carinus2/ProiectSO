#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <caracter>"
    exit 1
fi

caracter=$1
contor=0

while read -r line; do
    echo "Procesare linie: '$line'"
    if [[ $line =~ ^[A-Z] ]]; then
        if [[ $line =~ [?.!]$ ]]; then
            if ! [[ $line =~ ,\ si\  || $line =~ \ si,\  ]]; then
                if [[ $line == *"$caracter"* ]]; then
                    ((contor++))
                fi
            fi
        fi
    fi
done

echo "Numarul de propozitii corecte care contin caracterul $caracter este : $contor"
