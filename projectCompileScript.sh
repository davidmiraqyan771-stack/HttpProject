#! /bin/sh

cd ./server

gcc serverHTTP.c ./thread/clientThread.c ./serverUtils/serverUtils.c -L../utils -lutils -o server

cd ../client

gcc clientHTTP.c -L../utils -lutils -o client