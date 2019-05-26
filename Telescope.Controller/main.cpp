#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

//#include <wiringPi.h>

  // 1 => 25
  // 4 => 100
  // 8 => 200
int movespeedy = 1;

// 1 => 200
// 2 => 100
// 4 => 50
// 8 => 25
int movespeedx = 8;


int open_serial();
long int findSize(const char* file_name);

int cur_x = 0;

void initTelescope(int serialSocket)
{
  write(serialSocket, "i", 1);

  sleep(20);

  //TODO: wait for HOMED

  write(serialSocket, "u", 1);

  for (int i = 0; i < 80; i++)
  {
    write(serialSocket, "v", 1);
    usleep(100000);
  }

  sleep(1);
}

int gather_data()
{
  int pid = fork();

  if (pid == 0) // new process
  {
    char filename[80];
    sprintf(filename, "./%d.dump", cur_x);

    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    dup2(fd, 1);
    close(fd);

    execl("/usr/bin/hackrf_sweep", "hackrf_sweep", "-B", (char *)NULL);
    _exit(1);
    // does not return
    return 0;
  }
  else // main process
  {
    return pid;

    // move to end
    
  }

  printf("x: %d\n", cur_x);
}

void stop_gather(int pid)
{
  //int status = 0;
  //int pid2;

  //for (int i = 0; i < 50; i++)
  //{
  //  pid2 = waitpid(pid, &status, WNOHANG);

  //  if (pid2 != 0)
  //    return true;

  //  usleep(100000);
  //}

  kill(pid, SIGTERM);
  usleep(100000);
}

bool do_lane(int serialSocket)
{
  //const char* direction = up ? "u" : "d";

  write(serialSocket, "d", 1);
  usleep(100000);

  int pid = gather_data();

  for (int i = 0; i < 200; i++)
  {
    write(serialSocket, "v", 1);
    usleep(movespeedy * 100000);
  }

  stop_gather(pid);

  write(serialSocket, "u", 1);
  usleep(100000);

  for (int i = 0; i < 200; i++)
  {
    write(serialSocket, "v", 1);
    usleep(25000);
  }

  //if (i != 199)
  //{
  //  write(serialSocket, "v", 1);
  //  usleep(100000);
  //  cur_y += up ? 1 : -1;
  //}
  //else // last
  //{
  //  for (int i = 0; i < 2; i++)
  //  {
  //    write(serialSocket, "h", 1);
  //    usleep(100000);
  //  }

  //  ;
  //}

  char filename[80];
  sprintf(filename, "./%d.dump", cur_x);

  if (findSize(filename) == 0)
  {
    return false;
  }


  for (int i = 0; i < movespeedx; i++)
  {
    write(serialSocket, "h", 1);
    usleep(500000);
  }

  cur_x++;

  return true;
}

long int findSize(const char* file_name)
{
  struct stat st; /*declare stat variable*/

  /*get the size using stat()*/

  if (stat(file_name, &st) == 0)
    return (st.st_size);
  else
    return -1;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Usage: <resolution>\n");
    printf("1 => 200\n");
    printf("2 => 100\n");
    printf("4 => 50\n");
    printf("8 => 25\n");

    return 0;
  }

  int resolution = 0;

  sscanf(argv[1], "%d", &resolution);

  switch (resolution)
  {
  case 8:
    movespeedy = 1;
    movespeedx = 8;
    break;

  case 4:
    movespeedy = 2;
    movespeedx = 4;
    break;

  case 2:
    movespeedy = 4;
    movespeedx = 2;
    break;

  case 1:
    movespeedy = 8;
    movespeedx = 1;
    break;

  default:
    printf("Usage: <resolution>\n");
    printf("1 => 200\n");
    printf("2 => 100\n");
    printf("4 => 50\n");
    printf("8 => 25\n");

    return 0;
  }

  int serialSocket = open_serial();

  if (serialSocket == -1)
  {
    /*    * Could not open the port.    */
    perror("open_port: Unable to open /dev/ttyf1 ? ");
  }
  else
  {
    initTelescope(serialSocket);

    for (int i = 0; i < (200/ movespeedx); i++) {
      while (!do_lane(serialSocket))
        ;
    }

    close(serialSocket);
  }
  return 0;
}

int open_serial()
{
  /*
   * Oeffnet seriellen Port
   * Gibt das Filehandle zurueck oder -1 bei Fehler
   *
   * RS232-Parameter:
   * 19200 bps, 8 Datenbits, 1 Stoppbit, no parity, no handshake
   */

  int fd; /* Filedeskriptor */
  struct termios options; /* Schnittstellenoptionen */

  /* Port oeffnen - read/write, kein "controlling tty", Status von DCD ignorieren */
  fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd >= 0)
  {
    /* get the current options */
    fcntl(fd, F_SETFL, 0);
    if (tcgetattr(fd, &options) != 0) return (-1);
    memset(&options, 0, sizeof(options)); /* Structur loeschen, ggf. vorher sichern
                                         und bei Programmende wieder restaurieren */
    /* Baudrate setzen */
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    /* setze Optionen */
    options.c_cflag &= ~PARENB; /* kein Paritybit */
    options.c_cflag &= ~CSTOPB; /* 1 Stoppbit */
    options.c_cflag &= ~CSIZE; /* 8 Datenbits */
    options.c_cflag |= CS8;

    /* 19200 bps, 8 Datenbits, CD-Signal ignorieren, Lesen erlauben */
    options.c_cflag |= (CLOCAL | CREAD);

    /* Kein Echo, keine Steuerzeichen, keine Interrupts */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag = IGNPAR; /* Parity-Fehler ignorieren */
    options.c_oflag &= ~OPOST; /* setze "raw" Input */
    options.c_cc[VMIN] = 0; /* warten auf min. 0 Zeichen */
    options.c_cc[VTIME] = 0; /* Timeout 1 Sekunde */
    tcflush(fd, TCIOFLUSH); /* Puffer leeren */
    if (tcsetattr(fd, TCSAFLUSH, &options) != 0) return (-1);
  }
  return (fd);
}
