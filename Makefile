CC = clang
UNAME_S := $(shell uname -s)

CFLAGS = -Wall -Wextra -std=c11
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

SRCS = main.c physics.c render.c
OBJS = $(SRCS:.c=.o)
TARGET = main

# SDL2: se intenta primero con pkg-config y, si no esta disponible,
# con sdl2-config (comun en instalaciones via Homebrew/MSYS2).
SDL2_CFLAGS  := $(shell pkg-config --cflags sdl2 2>/dev/null || sdl2-config --cflags 2>/dev/null)
SDL2_LDFLAGS := $(shell pkg-config --libs sdl2 2>/dev/null || sdl2-config --libs 2>/dev/null)

DEMO_SRCS = render.c render_demo.c
DEMO_OBJS = $(DEMO_SRCS:.c=.o)
DEMO_TARGET = render_demo

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(SDL2_LDFLAGS)

%.o: %.c screensaver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Los .o que usan SDL necesitan sus propios flags de include/link;
# estas reglas explicitas tienen prioridad sobre el patron generico de arriba.
main.o: main.c render.h screensaver.h
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -c main.c -o main.o

render.o: render.c render.h screensaver.h
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -c render.c -o render.o

render_demo.o: render_demo.c render.h screensaver.h
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -c render_demo.c -o render_demo.o

# make render_demo: compila el binario de prueba del modulo de render
# (ventana + cuerpos "mock" rebotando), sin depender de main.c/physics.c.
$(DEMO_TARGET): $(DEMO_OBJS)
	$(CC) $(DEMO_OBJS) -o $(DEMO_TARGET) $(LDFLAGS) $(SDL2_LDFLAGS)

# make omp: mismo build pero con OpenMP habilitado (para cuando haya #pragma omp)
omp: CFLAGS += $(OMP_CFLAGS)
omp: LDFLAGS += $(OMP_LDFLAGS)
omp: $(TARGET)

run: $(TARGET)
	./$(TARGET)

run-demo: $(DEMO_TARGET)
	./$(DEMO_TARGET)

clean:
	rm -f $(OBJS) $(DEMO_OBJS) $(TARGET) $(DEMO_TARGET)

.PHONY: run run-demo clean omp render_demo