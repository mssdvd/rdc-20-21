/* - Funzionalità aggiunte
 *   - Porta univoca
 *   - Lettura Content Length
 *   - Mettersi in ascolto per la richiesta POST
 *   - Lettura entity body post
 *   - Parsing stringa POST
 *   - Composisizione del commando da passare alla shell
 *   - Esecuzione del commando tramite popen(3) per leggere la stdout
 *   - Scrittura della response
 *
 * - Punti di intervento nel programma
 *   - Modificata porta per renderla univoca
 *   - Aggiunta sezione condionale per richieste POST che richiedono di
 *     accedere alla risorsa /cgi-bin/command
 *     - In questa sezione vengono effettuate le operazioni descritte nella
 *       sezione precedente
 *   - Controllo che la sezione che trasmette i file richiesti tramite GET non
 *     venga eseguita in seguito alla gestione di una POST
 *     - Senza il controllo inserirebbe una 'HTTP/1.1 200 OK' alla fine della
 *       response
 *
 * - Eventuali scelte implementative
 *   - A differenza di quanto visto a lezione si sfrutta lo piping
 *
 * - Descrizione dell'esperimento
 *   - In base ai dati contenuti nei 3 form il sw esegue le operazioni richieste
 *
 * - Descrizione dell'esito e verifica correttezza
 *   - Il sw restituisce corretamente l'output del programma
 *   - Funziona anche form vuoti
 *
 * BUG:
 * - I dati di input non vengono sanificati (in particolare spazi o
 *   enventuali altri carattereri intrepretabili dalla shell)
 * - Non c'è un controllo che i parametri non siano commandi
 * - Nessun controllo che TUTTI i form siano non vuoti.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

int tmp;

struct header {
  char *n;
  char *v;
} h[100];

int main() {
  struct sockaddr_in addr, remote_addr;
  int i, j, k, s, t, s2, len;
  int content_len = 0;
  char command[100];
  int c;
  FILE *fin;
  int yes = 1;
  char *commandline;
  char *method, *path, *ver;
  char *cmd, *param1, *param2;
  char request[5000], response[10000];
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("Socket fallita");
    return 1;
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7382);
  addr.sin_addr.s_addr = 0;
  t = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (t == -1) {
    perror("setsockopt fallita");
    return 1;
  }
  if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    perror("bind fallita");
    return 1;
  }
  if (listen(s, 225) == -1) {
    perror("Listen Fallita");
    return 1;
  }
  len = sizeof(struct sockaddr_in);
  while (1) {
    s2 = accept(s, (struct sockaddr *)&remote_addr, &len);
    if (s2 == -1) {
      perror("Accept Fallita");
      return 1;
    }
    bzero(h, 100 * sizeof(struct header *));
    commandline = h[0].n = request;
    for (j = 0, k = 0; read(s2, request + j, 1); j++) {
      if (request[j] == ':' && (h[k].v == 0)) {
        request[j] = 0;
        h[k].v = request + j + 1;
      } else if ((request[j] == '\n') && (request[j - 1] == '\r')) {
        request[j - 1] = 0;
        if (h[k].n[0] == 0)
          break;
        h[++k].n = request + j + 1;
      }
    }
    printf("Command line = %s\n", commandline);
    for (i = 1; i < k; i++) {
      printf("%s ----> %s\n", h[i].n, h[i].v);
      if (strcmp(h[i].n, "Content-Length") == 0) {
        content_len = atoi(h[i].v);
      }
    }
    method = commandline;
    for (i = 0; commandline[i] != ' '; i++) {
    }
    commandline[i] = 0;
    path = commandline + i + 1;
    for (i++; commandline[i] != ' '; i++)
      ;
    commandline[i] = 0;
    ver = commandline + i + 1;
    /* il terminatore NULL dopo il token versione è già stato messo dal parser
     * delle righe/headers*/

    if (strcmp(method, "POST") == 0 && strcmp(path, "/cgi-bin/command") == 0) {
      printf("POST POST POST\n");
      int i = 0;
      for (j = 0; (j = read(s2, command + i, content_len - i)) > 0; i += j)
        ;
      command[i] = 0; // End of string
      printf("%s\n", command);

      // Parsing command
      i = 0;
      for (; command[i] != '='; i++)
        ;
      cmd = command + i + 1;
      for (i++; command[i] != '&'; i++)
        ;
      command[i] = 0;
      for (i++; command[i] != '='; i++)
        ;
      param1 = command + i + 1;
      for (i++; command[i] != '&'; i++)
        ;
      command[i] = 0;
      for (i++; command[i] != '='; i++)
        ;
      param2 = command + i + 1;
      for (i++; command[i] != 0; i++)
        ;
      command[i] = 0;

      printf("cmd=%s param1=%s param2=%s\n", cmd, param1, param2);

      char final_command[100];
      sprintf(final_command, "%s %s %s", cmd, param1, param2);

      FILE *pipe = popen(final_command, "r");

      sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
      write(s2, response, strlen(response));

      c = fgetc(pipe);
      write(s2, &c, 1);
      while ((c = fgetc(pipe)) != EOF)
        write(s2, &c, 1);
      pclose(pipe);
    }

    printf("method=%s path=%s ver=%s\n", method, path, ver);
    if (strncmp(path, "/cgi-bin/", 9) == 0) {
      t = system(command);
      if (t != -1)
        strcpy(path + 1, "tmpfile.txt");
    }
    if ((fin = fopen(path + 1, "rt")) == NULL) {
      sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
      write(s2, response, strlen(response));
    } else if (strcmp(method, "POST") != 0) {
      sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
      write(s2, response, strlen(response));
      while ((c = fgetc(fin)) != EOF)
        write(s2, &c, 1);
      fclose(fin);
    }
    close(s2);
  }
}
