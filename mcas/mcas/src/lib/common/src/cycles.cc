#include <assert.h>
#include <common/cycles.h>
#include <common/logging.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <boost/numeric/conversion/cast.hpp> 

namespace common
{


float get_rdtsc_frequency_mhz()
{
  float clock_speed_in_MHz = 0;
      
  if(::getenv("USE_MODULE_TSC")) {
    std::ifstream ifs("/sys/devices/system/cpu/cpu0/tsc_freq_khz");
    long f;
    ifs >> f;
    clock_speed_in_MHz = (boost::numeric_cast<float>(f) / 1000.0f);
  }
  else {
    FILE *fp;
    char buffer[1024];
    size_t bytes_read;
    char *match;
    float clock_speed = 0.0;
    
    /* Read the entire contents of /proc/cpuinfo into the buffer. */
    fp = fopen("/proc/cpuinfo", "r");
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);

    assert(bytes_read > 0);
    buffer[bytes_read] = '\0';

    /* First try to get cpu freq from model name. */
    match = strstr(buffer, "model name");
    assert(match);
    while (*match != '@') match++;
    /* Parse the line to extract the clock speed. */
    sscanf(match, "@ %f", &clock_speed);

    if (clock_speed > 0)
      clock_speed_in_MHz = clock_speed * 1000.0f;
    else {
      /* some qemu guest and the aep1 host don't have freq in model name */
      match = strstr(buffer, "cpu MHz");
      assert(match);
      while (*match != ':') match++;

      sscanf(match, ": %f", &clock_speed_in_MHz);
    }
    assert(clock_speed_in_MHz > 0);
  }
  
  return clock_speed_in_MHz;
}

float cycles_to_usec(cpu_time_t time)
{
  return float(time) / get_rdtsc_frequency_mhz();
}

}  // namespace common
