#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

//for file in `ls img1`; do ./projects/rfanalizer/bin/ARM/Debug/rfanalizer.out img1/$file img1/$file.hex ; done
//chmod 0644 img1hex/*
//tar -cjf img.tbz img1hex
//sudo obexftpd -v -b -c .

int main(int argc, char** argv)
{
  if (argc < 3)
  {
    printf("Please specify a file.\n");
    return 0;
  }

  const auto file_socket = open(argv[1], O_RDONLY);
  unsigned int len;

  float data[1200];

  while (true)
  {
    const auto bytes_read = read(file_socket, &len, sizeof(len));

    if (bytes_read <= 0)
      break;

    if (len != 36)
      break;

    long unsigned long freq_start, freq_end;
    read(file_socket, &freq_start, sizeof(freq_start));
    read(file_socket, &freq_end, sizeof(freq_end));

    float dbs[5];
    read(file_socket, dbs, sizeof(float) * 5);

    //printf("%llu %llu %f\n", freq_start/ 5000000, freq_end / 5000000, (dbs[0] + dbs[1] + dbs[2] + dbs[3] + dbs[4]) / 5.0f);

    data[freq_start / 5000000] = (dbs[0] + dbs[1] + dbs[2] + dbs[3] + dbs[4]) / 5.0f;
  }

  close(file_socket);

  const auto file_socket_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC);

  write(file_socket_out, data, sizeof (float) * 1200);

  close(file_socket_out);


  return 0;
}
