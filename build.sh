gcc -std=c99 -Wall -pedantic -march=native -Ofast -fomit-frame-pointer -flto -fuse-linker-plugin -fwhole-program $* bbchess.c
#clang -std=c99 -Wall -pedantic -march=native -O2 -fomit-frame-pointer $* bbchess.c

