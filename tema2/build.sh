#!/bin/bash
# build.sh - compileaza toate componentele din tema2/
# Ruleaza din directorul radacina al proiectului (parintele lui tema2/)

set -e

SRC="../src"
COMMON="common"
CFLAGS="-std=c++17 -O2 -pthread -I."

echo "=== Compilare broker ==="
g++ $CFLAGS broker.cpp \
    $COMMON/matcher.cpp \
    $COMMON/serialization.cpp \
    $SRC/utils.cpp \
    $SRC/subscription_generator.cpp \
    $SRC/publication_generator.cpp \
    -o broker

echo "=== Compilare subscriber ==="
g++ $CFLAGS subscriber.cpp \
    $COMMON/serialization.cpp \
    $SRC/utils.cpp \
    $SRC/subscription_generator.cpp \
    -o subscriber

echo "=== Compilare publisher ==="
g++ $CFLAGS publisher.cpp \
    $COMMON/serialization.cpp \
    $SRC/utils.cpp \
    $SRC/publication_generator.cpp \
    -o publisher


echo ""
echo "=== Compilare reusita! ==="
echo ""
echo "Pornire sistem (in terminale separate):"
echo "  ./broker 1"
echo "  ./broker 2"
echo "  ./broker 3"
echo ""
echo "Subscriberi (dupa brokeri):"
echo "  ./subscriber 1 10  # subscriber 1, 10 subscriptii"
echo "  ./subscriber 2 10"
echo "  ./subscriber 3 10"
echo ""
echo "Publisheri:"
echo "  ./publisher 1 0 100  # publisher 1, infinit, 100ms delay"
echo "  ./publisher 2 0 100"
