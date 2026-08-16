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

SRCS = main.c physics.c
OBJS = $(SRCS:.c=.o)
TARGET = main

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c screensaver.h
	$(CC) $(CFLAGS) -c $< -o $@

# make omp: mismo build pero con OpenMP habilitado (para cuando haya #pragma omp)
omp: CFLAGS += $(OMP_CFLAGS)
omp: LDFLAGS += $(OMP_LDFLAGS)
omp: $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: run clean omp
