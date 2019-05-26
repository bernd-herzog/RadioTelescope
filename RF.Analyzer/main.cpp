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
    char file_name_buf[300];
    sprintf(file_name_buf, "%s/%d.dump", argv[1], x);

    const auto file_socket = open(file_name_buf, O_RDONLY);
    if (file_socket <= 0)
    {
      printf("could not open %s\n", file_name_buf);
      return 0;
    }

    printf("parsing file %s\n", file_name_buf);
    
    //for (int y = 0; y < height; y++)
    {
      unsigned int len;
      float data[1200];
      for (size_t i = 0; i < 1200; i++)
      {
        data[i] = 0.0f;
      }

      int i = 0;
      int y = 0;

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
        if (data[freq_start / 5000000] != 0.0f)
        {
          write(file_socket_out, data, sizeof(float) * 1200);
          
          if (i != 1200)
          {
            printf("missing data: %d\n", i);
          }

          y++;

          if (y == height)
          {
            break;
          }

          i = 0;

          for (size_t j = 0; j < 1200; j++)
          {
            data[j] = 0.0f;
          }

        }

        data[freq_start / 5000000] = (dbs[0] + dbs[1] + dbs[2] + dbs[3] + dbs[4]) / 5.0f;
        i++;
      }

      if (i != 1200)
      {
        printf("missing data: %d\n", i);

      }
        
      printf("got: %d pixels\n", y);


      write(file_socket_out, data, sizeof(float) * 1200);

    }

    close(file_socket);
    printf("finished row %d\n", x);
  }

  close(file_socket_out);

  return 0;
}
