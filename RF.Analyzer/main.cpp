#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

//for file in `ls img1`; do ./projects/rfanalizer/bin/ARM/Debug/rfanalizer.out img1/$file img1/$file.hex ; done
//chmod 0644 img1hex/*
//tar -cjf img.tbz img1hex
//sudo obexftpd -v -b -c .

int main(int argc, char** argv)
{
  if (argc != 5)
  {
    printf("Usage: <input directory> <width> <height> <output file>\n");
    return 0;
  }

  const auto file_socket_out = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC);

  int width = 0;
  int height = 0;

  sscanf(argv[2], "%d", &width);
  sscanf(argv[3], "%d", &height);

  write(file_socket_out, &width, sizeof(int));
  write(file_socket_out, &height, sizeof(int));

  printf("Creating image with %d x %d.\n", width, height);

  for (int x = 0; x < width; x++)
  {
    for (int y = 0; y < height; y++)
    {
      char file_name_buf[300];
      sprintf(file_name_buf, "%s/%d-%d.dump", argv[1], x, y);
      
      const auto file_socket = open(file_name_buf, O_RDONLY);
      if (file_socket <= 0)
      {
        printf("could not open %s\n", file_name_buf);
        return 0;  
      }

      printf("parsing file %s\n", file_name_buf);

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

        write(file_socket_out, data, sizeof(float) * 1200);
      }

      close(file_socket);
    }

    printf("finished row %d\n", x);
  }

  close(file_socket_out);

  return 0;
}
