#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/types.h>
#include <sys/wait.h>

//#include <wiringPi.h>

// LED-PIN - wiringPi-PIN 0 ist BCM_GPIO 17.
// Wir müssen bei der Initialisierung mit wiringPiSetupSys die BCM-Nummerierung verwenden.
// Wenn Sie eine andere PIN-Nummer wählen, verwenden Sie die BCM-Nummerierung, und
// aktualisieren Sie die Eigenschaftenseiten – Buildereignisse – Remote-Postbuildereignisbefehl 
// der den GPIO-Export für die Einrichtung für wiringPiSetupSys verwendet.
//#define	LED	17

int open_serial();

int cur_x = 0;
int cur_y = 0;

void initTelescope(int serialSocket)
{
  write(serialSocket, "i", 1);

  sleep(20);

  //TODO: wait for HOMED

  write(serialSocket, "d", 1);

  for (int i = 0; i < 120; i++)
  {
    write(serialSocket, "v", 1);
    usleep(100000);
  }

  sleep(1);
}

bool gather_data()
{
  int pid = fork();

  if (pid == 0) // new process
  {
    char filename[80];
    sprintf(filename, "./%d-%d.dump", cur_x, cur_y);

    int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    dup2(fd, 1);
    close(fd);

    execl("/usr/bin/hackrf_sweep", "hackrf_sweep", "-1", "-B", (char *)NULL);
    //execl("/bin/echo", "echo", "echotest", "-1", "-B", (char *)NULL);
    return false;
  }
  else // main process
  {
    int status = 0;
    int pid2;

    for (int i = 0; i < 50; i++)
    {
      pid2 = waitpid(pid, &status, WNOHANG);

      if (pid2 != 0)
        return true;

      usleep(100000);
    }

    kill(pid, SIGTERM);
    usleep(100000);

    return false;
  }

  printf("x: %d\ty: %d\n", cur_x, cur_y);
}

void do_lane(int serialSocket, bool up)
{
  const char* direction = up ? "u" : "d";

  write(serialSocket, direction, 1);

  for (int i = 0; i < 200; i++)
  {
    while (!gather_data())
      ;

    write(serialSocket, "v", 1);
    usleep(100000);
    cur_y += up ? 1 : -1;
  }

  for (int i = 0; i < 2; i++)
  {
    write(serialSocket, "h", 1);
    usleep(100000);
  }

  cur_x++;
}

int main()
{
  int serialSocket = open_serial();

  if (serialSocket == -1)
  {
    /*    * Could not open the port.    */
    perror("open_port: Unable to open /dev/ttyf1 ? ");
  }
  else
  {
    initTelescope(serialSocket);

    for (int i = 0; i < 100; i++) {
      do_lane(serialSocket, true);
      do_lane(serialSocket, false);
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
