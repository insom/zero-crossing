#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <pulse/error.h>
#include <pulse/simple.h>

#define RATE 48000
#define BUFZ 10000

int main(int c, char **v) {
  pa_simple *s;
  pa_sample_spec ss;

  ss.format = PA_SAMPLE_S16NE;
  ss.channels = 1;
  ss.rate = RATE;

  int error;

  s = pa_simple_new(NULL, "ZeroCool", PA_STREAM_RECORD, NULL, "Music", &ss,
                    NULL, NULL, &error);
  if (!s) {
    fprintf(stderr, __FILE__ ": pa_simple_new() failed: %s\n",
            pa_strerror(error));
    return -1;
  }

  int16_t buf[BUFZ] = {0};

  int crossings = 0;
  int since_last_crossing = 0;
  double average_crossing = 0;
  double hz_guess = 0;
  bool was_negative = false;

  for (;;) {
    if (pa_simple_read(s, buf, BUFZ, &error) < 0) {
      fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n",
              pa_strerror(error));
      return -1;
    }

    for (int y = 0; y < BUFZ; y++) {
      int16_t s = buf[y];
      if ((s < 0 && !was_negative) || (s >= 0 && was_negative)) {
        crossings++;
        average_crossing = (average_crossing + since_last_crossing + since_last_crossing) /
                           3.0; // crude moving average
        hz_guess = 0.5 / (average_crossing / RATE);
        since_last_crossing = 0;
        was_negative = !was_negative;
      } else {
        since_last_crossing++;
      }
    }
    printf("MA of interval: %f, guess is %.02fHz\n", average_crossing,
           hz_guess);
    printf("There were %d crossings.\n", crossings);
    fflush(stdout);
  }
}
