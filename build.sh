gcc -std=c11 -Wall -pedantic -Winline -march=native -Ofast  -flto -fuse-linker-plugin $* bbchess.c
#clang -std=c99 -Wextra -march=native -O3 -fomit-frame-pointer -flto bbchess.c

