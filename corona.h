#ifndef CORONA_H
#define CORONA_H
#define CORONA_TEX_SIZE 128

typedef struct {
  unsigned char *pixels;
} CoronaBuffer;

int corona_init(CoronaBuffer *corona);
void corona_free(CoronaBuffer *corona);
void corona_generate(CoronaBuffer *corona, float time_seconds);

#endif
