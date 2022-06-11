#! /bin/bash

echo "creando la cartella per le tracce..."
$(mkdir tracks>&/dev/null)
if [[ $? -ne 0 ]]; then
    echo "errore creazione cartella per le tracce."
    exit -1
else
    echo "creata cartella per le tracce"
fi

echo ""
echo "compilando il programma..."
$(make)
if [[ $? -ne 0 ]]; then
    echo ""
    echo "errore nella compilazione."
else
    echo "compilato l'mp3blatta"
fi

