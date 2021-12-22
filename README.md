# i8080 Space Invaders Emulator

Space Invaders Emulator written entirely in C primarily inspired by http://emulator101.com/


## Depdendencies

- C compiler (clang used by default)
- libsdl2 
- libsdl2-ttf 

NOTE: Theoretically should be cross-platform but has only been tested on Ubuntu running on WSL2. 

## Running

1. `$ make`
2. `$ ./main path/to/rom`

NOTE: If you find a Space Invaders ROM with multiple files (.e, .f, .g, .h), then you want to pass a file containing the result of concatenating all of the files in reverse-alphabetical order, i.e.:
```
$ cat invaders.h > invaders    
$ cat invaders.g >> invaders    
$ cat invaders.f >> invaders    
$ cat invaders.e >> invaders
$ ./main invaders
```
