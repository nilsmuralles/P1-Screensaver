CC = clang
UNAME_S := $(shell uname -s)

CFLAGS_COMMON  = -Wall -Wextra -std=c11
CFLAGS_RELEASE = -O3
# -march=native da el mejor rendimiento en la maquina donde se compila, pero
# el binario deja de ser portable a CPUs distintas (y por tanto no es
# reproducible entre maquinas para comparar benchmarks). Se activa aparte:
#   make ARCH_NATIVE=1
ifeq ($(ARCH_NATIVE),1)
    CFLAGS_RELEASE += -march=native
endif
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
LDFLAGS = -lm

# macOS (clang de Apple) necesita apuntar a libomp de Homebrew a mano.
# Linux, WSL y MinGW/MSYS2 en Windows soportan -fopenmp de forma nativa.
ifeq ($(UNAME_S),Darwin)
    OMP_PREFIX := $(shell brew --prefix libomp 2>/dev/null)
    OMP_CFLAGS := -Xpreprocessor -fopenmp -I$(OMP_PREFIX)/include
    OMP_LDFLAGS := -L$(OMP_PREFIX)/lib -lomp
else
    OMP_CFLAGS := -fopenmp
    OMP_LDFLAGS := -fopenmp
endif

SRCS = main.c physics.c render.c spatial.c corona.c barnes_hut.c bench.c
OBJS = $(SRCS:.c=.o)
TARGET = main

# raylib se intenta primero con pkg-config
# Si no hay .pc, se cae a flags de link manuales segun el SO, que es
# el caso mas comun cuando raylib se compila/instala desde codigo fuente.
ifeq ($(UNAME_S),Darwin)
    RAYLIB_LDFLAGS_FALLBACK := -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
else ifneq (,$(findstring MINGW,$(UNAME_S)))
    RAYLIB_LDFLAGS_FALLBACK := -lraylib -lopengl32 -lgdi32 -lwinmm
else
    RAYLIB_LDFLAGS_FALLBACK := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

RAYLIB_CFLAGS  := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LDFLAGS := $(shell pkg-config --libs raylib 2>/dev/null)
ifeq ($(strip $(RAYLIB_LDFLAGS)),)
    RAYLIB_LDFLAGS := $(RAYLIB_LDFLAGS_FALLBACK)
endif

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(RAYLIB_LDFLAGS) $(OMP_LDFLAGS)

%.o: %.c screensaver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Los .o que usan raylib necesitan sus propios flags de include/link;
# estas reglas explicitas tienen prioridad sobre el patron de arriba.
main.o: main.c render.h screensaver.h spatial.h bench.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c main.c -o main.o

render.o: render.c render.h screensaver.h corona.h render_shaders.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c render.c -o render.o

# make omp: mismo build pero con OpenMP habilitado (para cuando haya #pragma omp)
omp: CFLAGS += $(OMP_CFLAGS)
omp: LDFLAGS += $(OMP_LDFLAGS)
omp: $(TARGET)

# physics.o, spatial.o, corona.o, barnes_hut.o y bench.o siempre se compilan
# con flags de OpenMP (independiente del target 'omp' de arriba), porque su
# fallback sin _OPENMP es solo un camino de compatibilidad, no el modo de uso real.
physics.o: physics.c screensaver.h
	$(CC) $(CFLAGS) $(OMP_CFLAGS) -c physics.c -o physics.o

spatial.o: spatial.c spatial.h screensaver.h
	$(CC) $(CFLAGS) $(OMP_CFLAGS) -c spatial.c -o spatial.o

corona.o: corona.c corona.h
	$(CC) $(CFLAGS) $(OMP_CFLAGS) -c corona.c -o corona.o

barnes_hut.o: barnes_hut.c barnes_hut.h screensaver.h
	$(CC) $(CFLAGS) $(OMP_CFLAGS) -c barnes_hut.c -o barnes_hut.o

bench.o: bench.c bench.h screensaver.h spatial.h barnes_hut.h
	$(CC) $(CFLAGS) $(OMP_CFLAGS) -c bench.c -o bench.o

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: run clean omp