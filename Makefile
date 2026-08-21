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

DEMO_SRCS = render.c render_demo.c
DEMO_OBJS = $(DEMO_SRCS:.c=.o)
DEMO_TARGET = render_demo

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(RAYLIB_LDFLAGS)

%.o: %.c screensaver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Los .o que usan raylib necesitan sus propios flags de include/link;
# estas reglas explicitas tienen prioridad sobre el patron de arriba.
main.o: main.c render.h screensaver.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c main.c -o main.o

render.o: render.c render.h screensaver.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c render.c -o render.o

render_demo.o: render_demo.c render.h screensaver.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -c render_demo.c -o render_demo.o

# make render_demo: compila el binario de prueba del modulo de render
# (ventana + cuerpos "mock" rebotando), sin depender de main.c/physics.c.
$(DEMO_TARGET): $(DEMO_OBJS)
	$(CC) $(DEMO_OBJS) -o $(DEMO_TARGET) $(LDFLAGS) $(RAYLIB_LDFLAGS)

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