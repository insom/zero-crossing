#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <pulse/error.h>
#include <pulse/simple.h>

#define RATE 48000
#define BUFZ 7200
#define CR 5

int main(int c, char **v) {
  pa_simple *s;
  pa_sample_spec ss;

  // ss.format = PA_SAMPLE_S16NE;
  ss.format = PA_SAMPLE_FLOAT32NE;
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

  float buf[BUFZ] = {0};

  double crossings = 0;
  double since_last_crossing = 0;
  double average_crossing = 0;
  double hz_guess = 0;
  bool was_negative = false;
  float previous_reading;

  for (;;) {
    if (pa_simple_read(s, buf, BUFZ, &error) < 0) {
      fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n",
              pa_strerror(error));
      return -1;
    }

    bool have_reading = false;
    for (int y = 0; y < BUFZ; y++) {
      float s = buf[y];
      if(fabs(s) < 0.10 && !have_reading) continue;
      have_reading = true;
      since_last_crossing++;
      if ((s < 0 && !was_negative)) {
        if(crossings++ == 0) {
          since_last_crossing -= (1 - (fabs(s) / (fabs(s) + previous_reading)));
        }
        if (crossings >= CR) {
          since_last_crossing += (1 - (fabs(s) / (fabs(s) + previous_reading)));
          average_crossing = since_last_crossing / crossings;
          hz_guess = 1 / (average_crossing / RATE);
          since_last_crossing = 0;
          crossings = 0;
        }
        was_negative = !was_negative;
      } else if (s >= 0 && was_negative) {
        was_negative = !was_negative;
      }
      previous_reading = s;
    }
    if(have_reading) {
    printf("MA of interval: %f, guess is %.02fHz, symbol = %d            \n", average_crossing,
           hz_guess, (int)((hz_guess - 1000) / .625));
    fflush(stdout);
    }
  }
}
