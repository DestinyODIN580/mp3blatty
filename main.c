#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ncurses.h>
#include <time.h>

#define L 500
#define WIDTH 60
#define HEIGHT 12

char *tracklist = "tracks.txt";          /* file con la lista delle tracce */
char *trackinfofile = "trackinfo.txt";   /* file con le info della traccia */
char *playlistfile = "playlistnames.txt";/* file con la lista delle playlist */
char *trackplay = "mpv ";                /* lettore */

char buffer[L]; /* buffer generico */

int stdscrx;    /* y di stdscr */
int stdscry;    /* x di stdscr */

int starty;     /* x di menu_win */
int startx;     /* y di menu_win */

int infox;      /* x di info_win */
int infoy;      /* y di info_win */

int trackplaying = 0;   /* flag */
int trackindex = -1;    /* indice della traccia corrente */

/* opzioni */
int n_options = 12;
char *options[12] =
{
  "<F1>  Play",
  "<F2>  Pause",
  "<F3>  Random",
  "<F4>  Refresh",
  "<F5>  Killall",
  "<F6>  Import ~/Downloads",
  "<F7>  New playlist",
  "<F8>  Playlist menu",
  "<F9>  Add songs to ply",
  "<F11> Prev",
  "<F12> Next",
  "<TAB> Quit",
};

int n_choices = 0;      /* numero di tracce */
char **choises = NULL;  /* matrice delle tracce */

int n_playlists;        /* numero di playlist */
char **playlists = NULL;/* matrice delle playlist */

WINDOW *menu_win;       /* finestra con la lista delle tracce */
WINDOW *info_win;       /* finestra con le info della traccia */
WINDOW *play_win;       /* finestra con le playlist */
WINDOW *opts_win;       /* finestra con le opzioni */

void print_menu (WINDOW *, int, char **, int, int, int); /* stampa un menu */
void createPlaylist (void);     /* crea una playlist vuota */
void displayplaylists (void);   /* mostra le playlist */
void playlistsinmenu (int);     /* stampa le tracce della playlist */
void addSongtoPlaylist (void);  /* aggiunge tracce a una playlist esistente */

char **choisesinit (void);      /* inizializzatore delle tracce */
char **playlistinit (void);     /* inizializzatore delle playlist */
char **matrixgenerator (char *, char **);   /* genera matrici da file */

int system (char const *);
int scanplaylists (void);        /* controlla per nuove playlist */

int main (void)
{

    unsigned long int randomnum;    /* numero casuale */
    unsigned int rand;

    int highlight = 1;  /* cursore */
    int choice = 0;     /* traccia scelta */
    int isPaused;
    int c;              /* carattere in input */
    int i;              /* contatori */

    FILE *fInfo;        /* file delle informazioni */

    randomnum = 1;
    choice = 0;
    

    /* inzializzazione di ncurses */
    initscr();
    clear();
    noecho();
    curs_set(0);
    cbreak();

    getmaxyx (stdscr, stdscry, stdscrx);

    /* creazione di menu_win */
    startx = (stdscrx - WIDTH) / 2;
    starty = (stdscry - HEIGHT) / 2;
    menu_win = newwin (HEIGHT, WIDTH, starty, startx);

    /* creazione di info_win */
    infox = startx;
    infoy = starty + HEIGHT + 1;
    refresh ();
    info_win = newwin (4, WIDTH, infoy, infox);
    box (info_win, 0, 0);
    wrefresh (info_win);

    keypad (menu_win, TRUE);
    nodelay (stdscr, TRUE);
    nodelay (menu_win, TRUE);
    nodelay (play_win, TRUE);


    mvprintw (0, 0, "Mp3blatty");// by Devilisse");
    mvprintw (stdscry - 1, stdscrx - 7, "v. 1.0");
    refresh ();

    /* stampo il menu delle opzioni */
    opts_win = newwin (n_options + 2, 30, starty, startx + WIDTH + 1);
    box (opts_win, 0, 0);
    for (i = 0; i < n_options; i++)
        mvwprintw (opts_win, i + 1, 1, "%s", *(options + i));
    wrefresh (opts_win);

    /* creo il file con le tracce */
    strcpy (buffer, "touch ");
    strcat (buffer, tracklist);
    system (buffer);
    choises = choisesinit ();

    strcpy (buffer, "touch ");
    strcat (buffer, playlistfile);
    system (buffer);
    playlists = playlistinit ();

    for (isPaused = -1; ; )
    {
        wattron (menu_win, A_BOLD);
        wmove (menu_win, 1, 1);
        wclrtoeol (menu_win);
        wprintw (menu_win,"All songs:");
        wattroff (menu_win, A_BOLD);
        box (menu_win, 0, 0);

        /* genero un numero casuale */
        randomnum = randomnum * 1103515245 + 12345;
        rand = (unsigned int) (randomnum / 65536) % (n_choices + 1);

        /* pulisco la barra degli avvertimenti */
        if ((c = wgetch (menu_win)) != ERR)
        {
            move (starty - 1, startx);
            clrtoeol ();
           refresh ();
        }

        switch (c)
        {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;

            case KEY_DOWN:
                if(highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;

            /* play/pause */
            case ' ':
                if (!trackplaying)
                {
                    trackindex = choice = highlight;
                    isPaused = 0;
                    c = '\n';
                }
                else if (isPaused)
                {
                    system ("echo \'{ \"command\": [\"set_property\", \"pause\", false] }' | socat \
                    - /tmp/mpvsocket >& /dev/null");
                    isPaused = 0;
                }
                else
                {
                    system ("echo \'{ \"command\": [\"set_property\", \"pause\", true] }' | socat \
                    - /tmp/mpvsocket >& /dev/null");
                    isPaused = 1;
                }
                break;

            /* play */
            case KEY_F(1):
                system ("echo \'{ \"command\": [\"set_property\", \"pause\", false] }' | socat \
                - /tmp/mpvsocket >& /dev/null");
                isPaused = 0;
                break;

            /* pause */
            case KEY_F(2):
                system ("echo \'{ \"command\": [\"set_property\", \"pause\", true] }' | socat \
                - /tmp/mpvsocket >& /dev/null");
                isPaused = 1;
                break;

            /* traccia casuale */
            case KEY_F(3):
                if (rand > 0)
                    highlight = choice = trackindex = rand;
                else
                    highlight = choice = trackindex = 1;
                c = '\n';
                break;

            /* spostamento delle tracce da Downloads a tracks */
            case KEY_F(6):
                system ("pd=$(pwd); cd $HOME/Downloads; mv *.mp4 $pd/tracks >& \
                /dev/null");
                /* non mettere break e'volontario, come l'ordine dei case */

            /* refresh del file delle tracce */
            case KEY_F(4):
                system ("killall mpv >& /dev/null");
                choisesinit (); 
                highlight = 1;
                trackplaying = 0;
                break;

            /* uccisione processi mpv */
            case KEY_F(5):
                system ("killall mpv >& /dev/null");
                trackplaying = 0;
                break;

            /* nuova playlist */
            case KEY_F(7):
                createPlaylist ();
                break;

            /* apre la play_win */
            case KEY_F(8):
            case KEY_LEFT:
            case KEY_RIGHT:
                if (scanplaylists ())
                    displayplaylists ();
                else
                {
                    mvprintw (starty - 1, startx, "Nessuna playlist trovata");
                    refresh ();
                }
                break;

            /* aggiunge canzoni alla playlist esistente */
            case KEY_F(9):
                addSongtoPlaylist ();
                continue;
                break;

            /* traccia precedente */
            case KEY_F(11):
                if (highlight == 1)
                    highlight = choice = trackindex = n_choices;
                else
                    highlight = choice = trackindex = trackindex - 1;
                c = '\n';
                break;

            /* traccia successiva */
            case KEY_F(12):
                if (highlight == n_choices)
                    highlight = choice = trackindex = 1;
                else
                    highlight = choice = trackindex += 1;
                c = '\n';
                break;

            /* uscita dal programma */
            case '\t':
                choice = -1;
                break;

            /* selezione della traccia */
            case '\n':
                trackindex = choice = highlight;
                isPaused = 0;
                break;

            default:
                wrefresh (menu_win);
                break;
    }

    /* stampa della traccia highlight in info_win */
    wmove (info_win, 1, 2);
    wclrtoeol (info_win);
    box (info_win, 0, 0);
    wprintw (info_win, "%d/%d", highlight, n_choices);
    wmove (info_win, 1, (WIDTH - 9) / 2);

    if (!isPaused)
        wprintw (info_win, "Playing");
    else if (isPaused == 1)
        wprintw (info_win, "Paused");


    /* se una traccia e' riprodotta stampo il suo indice */
    if (trackplaying /* && trackindex != -1 */)
    {
        wattron (info_win, A_BLINK);
        if (trackindex < 10)
        {
          wmove (info_win, 1, WIDTH - 6);
          wprintw (info_win, "%d/%d", trackindex, n_choices);
        }
        else if (trackindex < 100)
        {
          wmove (info_win, 1, WIDTH - 7);
          wprintw (info_win, "%d/%d", trackindex, n_choices);
        }
        /* ... */
        wattroff (info_win, A_BLINK);
    }
    else
    {
        wmove (info_win, 1, (WIDTH - 9) / 2);
        wclrtoeol (info_win);
        wmove (info_win, 2, 2);
        wclrtoeol (info_win);
        box (info_win, 0, 0);
    }
    wrefresh (info_win);

    /* ristampa tutto per tenere aggiornato */
    print_menu (menu_win, highlight, choises, n_choices, HEIGHT, WIDTH);

    /* esco dal programma */
    if (choice == -1)
        break;

    /* controllo fine traccia */
    if (trackplaying)
    {
        fInfo = fopen (trackinfofile, "r");
        fseek (fInfo, -6, SEEK_END);
        for (i = 0; i < 4; i++)
        buffer[i] = fgetc (fInfo);
        buffer[i] = '\0';
        fclose (fInfo);

        /* fine traccia */
        if (!strcmp (buffer, "file"))
        {
            trackplaying = 0;
            *buffer = '\0';
            c = '\n';

        /* scorro alla traccia successiva */
        if (choice < n_choices)
        {
            choice++;
            highlight = trackindex = choice;
        }
        else 
            highlight = choice = trackindex = 1;
        }
    }

    /* traccia selezionata */
    if (c == '\n')
    {
        trackplaying = 1;
        isPaused = 0;

        system ("killall mpv >& /dev/null");

        /* resetto il file */
        fInfo = fopen (trackinfofile, "w");
        fclose (fInfo);

        /* mpv --flags... tracks/"%s.mp4"> trackinfo.txt & */
        strcpy (buffer, trackplay);
        strcat (buffer, " --input-ipc-server=/tmp/mpvsocket tracks/\"");
        strcat (buffer, choises[choice - 1]);
        strcat (buffer, "\" >& ");
        strcat (buffer, trackinfofile);
        strcat (buffer, " &");
        system (buffer);

        /* stampo la traccia avviata in info_win */
        wmove (info_win, 2, 2);
        wclrtoeol (info_win);
        box (info_win, 0, 0);
        wattron (info_win, A_BLINK);
        wprintw (info_win, "-");
        wattroff (info_win, A_BLINK);
        wprintw (info_win, " ");


        for (i = 0; choises[choice - 1][i] != '\0' && i < (WIDTH - 10); i++)
            wprintw (info_win, "%c", choises[choice - 1][i]);

        if (strlen (choises[choice - 1]) > (WIDTH - 10))
            wprintw (info_win, "...");
        wrefresh (info_win);
    }
    wrefresh (menu_win);
  }

  /* termino tutti i processi mpv */
  system ("killall mpv");

  /* rimuovo i file temporanei */
  strcpy (buffer, "rm ");
  strcat (buffer, trackinfofile);
  system (buffer);

  strcpy (buffer, "rm ");
  strcat (buffer, tracklist);
  system (buffer);

  strcpy (buffer, "rm ");
  strcat (buffer, playlistfile);
  system (buffer);

  /* dealloco tutta la memoria per ncurses */
  endwin();


  return 0;
}

char **choisesinit (void)
{
    char *localBuffer;
    int c;              /* carattere */
    int i, j;           /* contatori */
    
    FILE *f;            /* file */


    *buffer = '\0';
    if ((f = fopen (tracklist, "w")) == NULL)
    {
        perror ("Errore nell'apertura del file delle tracce");
        endwin ();
        exit (-1);
    }

    /* TODO: 1. se non esitono file .mp4 2. includere altri formati */
    /* creazione del file contenenti i titoli */
    strcpy (buffer, "(cd tracks/ && ls *.mp4 > ../");
    strcat (buffer, tracklist);
    strcat (buffer, " && cd ..)");
    system (buffer);
    fflush (f);
    fclose (f);

    /* riapro il file delle tracce in lettura */
    f = fopen (tracklist, "r");

    /* provvisto che non sia un file vuoto... */
    for (i = 0; (c = fgetc (f)) != EOF;)
        if (c == '\n')
            i++;

    /* alloco parte della matrice */
    choises = malloc (sizeof (char *) * (i + 1));
    localBuffer = malloc (sizeof (char) * L);

    /* riavvolgo e comincio la lettura */
    rewind (f);
    for (j = i = 0; 1 ; i++)
    {
        c = fgetc (f);
        if (c != '\n' && c != EOF)
            localBuffer[i] = c;
        else
        {
            /* fine file */
            if (c == EOF)
                break;

            localBuffer[i] = '\0';

            /* alloco il vettore e copio il nome della traccia */
            *(choises + j) = malloc (sizeof (char) * (i + 1));
            strcpy (*(choises + j), localBuffer);

            j++;
            i = -1;
       }
    } 
    *(choises + j) = '\0';
    fclose (f);
    n_choices = j;


    return choises;
}

char **playlistinit (void)
{
    char *localBuffer;

    int c;
    int i, j;

    FILE *f;
     

    /* riapro il file delle tracce in lettura */
    f = fopen (playlistfile, "r");

    for (i = 0; (c = fgetc (f)) != EOF; )
        if (c == '\n')
            i++;

    /* alloco parte della matrice */
    playlists = malloc (sizeof (char *) * (i + 1));
    localBuffer = malloc (sizeof (char) * L);

    /* riavvolgo e comincio la lettura */
    rewind (f);
    for (j = i = 0; 1 ; i++)
    {
        c = fgetc (f);

        if (c != '\n' && c != EOF)
            localBuffer[i] = c;
        else
        {
            /* fine file */
            if (c == EOF)
                break;

            localBuffer[i] = '\0';

            /* alloco il vettore e copio il nome della traccia */
            *(playlists + j) = malloc (sizeof (char) * (i + 1));
            strcpy (*(playlists + j), localBuffer);

            j++;
            i = -1;
        }
    }
    *(playlists + j) = '\0';
    fclose (f);
    n_playlists = j;
    free (localBuffer);


    return playlists;
}

char **matrixgenerator (char *s, char **m)
{
    char *localBuffer;

    int c;
    int i, j;

    FILE *f;


       
    /* riapro il file delle tracce in lettura */
    f = fopen (s, "r");

    /* provvisto che non sia un file vuoto... */
    for (i = 0; (c = fgetc (f)) != EOF;)
        if (c == '\n')
            i++;
    
    /* alloco parte della matrice */
    m = malloc (sizeof (char *) * (i + 1));
    localBuffer = malloc (sizeof (char) * L);

    /* riavvolgo e comincio la lettura */
    rewind (f);
    for (j = i = 0; 1 ; i++)
    {
        c = fgetc (f);
        if (c != '\n' && c != EOF)
            localBuffer[i] = c;
        else
        {
            /* fine file */
            if (c == EOF)
                break;

            localBuffer[i] = '\0';

            /* alloco il vettore e copio il nome della traccia */
            *(m + j) = malloc (sizeof (char) * (i + 1));
            strcpy (*(m + j), localBuffer);

            j++;
            i = -1;
        }
    } 
    *(m + j) = '\0';
    fclose (f);
    free (localBuffer);


    return m; 
}

void print_menu (WINDOW *win, int highlight, char **m, int len, int height, int width)
{
    int x, y, i, k, j;
    int cols;
    int top, bottom, range;

    x = y = 2;
    i = highlight - 1;

    range = width - 2 - 4 - 1;
    
    move (stdscry - 1, 0);
    clrtoeol ();
    //printw ("%d/%d", width, range);

    /* menu'statico / ultime HEIGHT scelte */
    //if (len <= (i + HEIGHT - 4)) //|| len < HEIGHT)
    //if (HEIGHT < (len - i))

    if (highlight > (len - height + 4) || len <= height - 4)
    {
        /* n. tracce dopo i */
        if (len <= height - 4)
            top = len - highlight + 1;
        else
            top = len - i;

        /* n tracce prima i */
        if (len <= height - 4)
            bottom = highlight - 1;
        else
            bottom = height - 4 - top;
        if (bottom < 0)
            bottom = 0;

        /* stampa delle tracce prima dell'highlight */
        for (j = i - bottom, k = 0; k < bottom; k++, j++, y++)
        {
            wmove (win, y, x);
            wclrtoeol (win);
            box (win, 0, 0);
            wprintw (win, "%2d  ", j + 1);

            for (cols = 0; m[j][cols] != '\0' && cols < (range - 3); cols++)
                wprintw (win, "%c", m[j][cols]);

            if (strlen (m[j]) >= (range - 3))
                wprintw (win, "...");
        }

        /* stampa della traccia nell'highlight */
        wmove (win, y, x);
        wclrtoeol (win);
        box (win, 0, 0);
        wprintw (win, "%2d ", i + 1);
        wattron (win, A_REVERSE);
        wprintw (win, " * ");

        for (cols = 0; m[i][cols] != '\0' && cols < (range - 6); cols++)
            wprintw (win, "%c", m[i][cols]);

        if (strlen (m[i]) >= (range - 6))
            wprintw (win, "...");
        wattroff (win, A_REVERSE);
        printw ("--> %s", m[i]);
        refresh ();
        y++;

        /* stampa delle tracce dopo l'highlight */
        for (k = 1; k < top; k++, y++)
        {
            wmove (win, y, x);
            wclrtoeol (win);
            box (win, 0, 0);

            wprintw (win, "%2d  ", i + k + 1);

            for (cols = 0; m[i + k][cols] != '\0' && cols < (range - 3); cols++)
                wprintw (win, "%c", m[i + k][cols]);

            if (strlen (m[i + k]) >= (range - 3))
                wprintw (win, "...");
        }
    }
    /* len > HEIGHT e non siamo alle ultime HEIGHT scelte */
    else
    {
        for (i = highlight - 1, k = 0; k < height - 4 && k < len; i++, k++, y++)
        {
            wmove (win, y, x);
            wclrtoeol (win);
            box (win, 0, 0);
            wprintw (win, "%2d ", i + 1);

            if (i + 1 == highlight)
            {
                wattron (win, A_REVERSE);
                wprintw (win, " * ");

                for (cols = 0; m[i][cols] != '\0' && cols < (range - 6); cols++)
                    wprintw (win, "%c", m[i][cols]);

                if (strlen (m[i]) >= (range - 6))
                    wprintw (win, "...");

                wattroff (win, A_REVERSE);
                printw ("--> %s", m[i]);
                refresh ();
            }
            else
            {
                wclrtoeol (win);
                box (win, 0, 0);
                wprintw (win, " ");

                for (cols = 0; m[i][cols] != '\0' && cols < (range - 3); cols++)
                    wprintw (win, "%c", m[i][cols]);

                if (strlen (m[i]) >= (range - 3))
                    wprintw (win, "...");
            }
        }
    }


    box (win, 0, 0);
    wrefresh (win);
    return ;
}

void createPlaylist (void)
{
    char *line = "Inserire il nome della playlist: ";

    int i, c;

    FILE *f;


    echo ();
    nodelay (stdscr, FALSE);
    curs_set(1);

    getmaxyx (stdscr, stdscry, stdscrx);

    /* creazione di menu_win */
    startx = (stdscrx - WIDTH) / 2;
    starty = (stdscry - HEIGHT) / 2;

    move (starty - 1, startx);
    clrtoeol ();


    /* stampa del prompt di inserimento */
    printw (line);
    for (i = 0, *buffer = '\0'; (c = wgetch (stdscr)) != '\n'; i++)
    {
        switch (c)
        {
            case '\b':
            case 127:
            case KEY_BACKSPACE:
                buffer[i - 1] = '\0';
                i -= 2;
                break;

            default:
                buffer[i] = c;
                buffer[i + 1] = '\0';
                break;
        }

        move (starty - 1, startx);
        clrtoeol ();
        printw (line);
        printw (buffer);
        refresh ();

        /* BACKSPACE quando non ci sono caratteri... */
        if (i < -1)
            i = -1;
    }

    noecho ();
    curs_set (0);
    nodelay (stdscr, TRUE);
    move (starty - 1, startx);
    clrtoeol ();

    if (!i || *buffer == '\0')
    {
        printw ("Nome vuoto");
        return ;
    }

    /* suffisso .ply se non gia'inserito */
    if (strstr (buffer, ".ply") == NULL)
        strcat (buffer, ".ply");

    /* crea file .ply */
    if ((f = fopen (buffer, "w")) != NULL)
        printw ("Playlist creata correttamente");
    else
        printw ("Errore nella creazione della playlist");

    fclose (f);
    refresh ();


    return ;
}

int scanplaylists (void)
{
    int c;
    int i, j;

    FILE *f;


    /* lista le playlist nella cartella */
    strcpy (buffer, "ls *.ply >& ");
    strcat (buffer, playlistfile);
    system (buffer);


    /* caso in cui non esistano le playlist */
    f = fopen (playlistfile, "r");
    for (i = 0; (c = fgetc (f)) != '\n'; i++)
        buffer[i] = c;
    buffer[i] = '\0';

    if (strstr (buffer, "ls: "))
    {
        strcpy (buffer, "rm ");
        strcat (buffer, playlistfile);
        system (buffer);
        fclose (f);
        return 0;
    }

    for (i = 0; (c = fgetc (f)) != EOF;)
        if (c == '\n')
            i++;

    /* alloco parte della matrice */
    playlists = malloc (sizeof (char *) * (i + 1));

    /* riavvolgo e comincio la lettura */
    rewind (f);
    for (j = i = 0; 1 ; i++)
    {
        c = fgetc (f);

        if (c != '\n' && c != EOF)
            buffer[i] = c;
        else
        {
            /* fine file */
            if (c == EOF)
                break;

            buffer[i] = '\0';

            /* alloco il vettore e copio il nome della traccia */
            *(playlists + j) = malloc (sizeof (char) * (i + 1));
            strcpy (*(playlists + j), buffer);

            j++;
            i = -1;
        }
    }
    *(playlists + j) = '\0';
    fclose (f);
    n_playlists = j;


    return 1;
}

void displayplaylists ()
{
    int highlight;
    int c;
    int j, k;
    int xMenu, yMenu;
    int additionalHeight;

    FILE *f;

    additionalHeight = 5;

    play_win = newwin (HEIGHT + additionalHeight, startx - 2, starty, 1);
    wrefresh (play_win);

    wattron (play_win, A_BOLD);
    mvwprintw (play_win, 1, 1, "Playlists:");
    wattroff (play_win, A_BOLD);
    box (play_win, 0, 0);
    wrefresh (play_win);

    nodelay (play_win, TRUE);
    keypad (play_win, TRUE);
    for (highlight = 1; ; )
    {
        c = wgetch (play_win);

        switch (c)
        {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_playlists;
                else
                    --highlight;
                break;

            case KEY_DOWN:
                if (highlight == n_playlists)
                    highlight = 1;
                else
                    ++highlight;
                break;

            case KEY_RIGHT:
                playlistsinmenu (highlight);
                break;

            case KEY_LEFT:
            case '\t':
                return ;
                break;

            default:
               break;
        }
        
        print_menu (play_win, highlight, playlists, n_playlists, HEIGHT + additionalHeight, startx - 2);
        box (play_win, 0, 0);
        wrefresh (play_win);

        f = fopen (playlists[highlight - 1], "r");

        werase (menu_win);
        box (menu_win, 0, 0);

        wattron (menu_win, A_BOLD);
        mvwprintw (menu_win, 1, 2, "%s", playlists[highlight - 1]);
        wattroff (menu_win, A_BOLD);

        for (j = k = 0, xMenu = yMenu = 2; k < HEIGHT - 4; j++)
        {
            c = fgetc (f);

            if (c == EOF)
                break;
            else if (c == '\n' || j == (WIDTH - 13))
            {
                buffer[j] = '\0';
                wmove (menu_win, yMenu, xMenu);
                wprintw (menu_win, "%2d  %s", k + 1, buffer);

                if (j == (WIDTH - 13))
                {
                    wprintw (menu_win, "...");

                    while ((c = fgetc (f)) != '\n' || c == EOF);

                    if (c == EOF)
                        break;
                }
                j = -1;
                k++;
                yMenu++;
            }
            else
                buffer[j] = c;
            }
            wrefresh (menu_win);

            rewind (f);
            for (j = 0; (c = fgetc (f)) != EOF; )
                if (c == '\n')
                    j++;

            for (k = 0; k < 7; k++)
                mvwprintw (info_win, 1, 3 + k, " ");
            mvwprintw (info_win, 1, 2, "0/%d", j);
            wrefresh (info_win);

            fclose (f);
        }


    return ;
}


void playlistsinmenu (int track)
{
    char **songs;

    int c;
    int highlight;
    int n_tracks;

    FILE *f;

    songs = NULL;


    songs = matrixgenerator (playlists[track - 1], songs);
    
    for (n_tracks = 0; *(songs + n_tracks) != NULL; n_tracks++);

    if (!n_tracks)
    {
        move (starty - 1, startx);
        clrtoeol ();
        printw ("Nessuna canzone nella playlist");
        refresh ();
        return ;
    }

    for (highlight = 1; ; )
    {
        switch (c = wgetch (menu_win))
        {
            case KEY_UP:
                if (highlight == 1)
                    highlight = n_tracks;
                else
                    --highlight;
                break;

            case KEY_DOWN:
                if (highlight == n_tracks)
                    highlight = 1;
                else
                    ++highlight;
                break;

            case '\n':
                system ("killall mpv >& /dev/null");

                f = fopen (trackinfofile, "w");

                /* mpv --flags... tracks/"%s.mp4"> trackinfo.txt & */
                strcpy (buffer, trackplay);
                strcat (buffer, " --input-ipc-server=/tmp/mpvsocket tracks/\"");
                strcat (buffer, *(songs + highlight - 1));
                strcat (buffer, "\" >& ");
                strcat (buffer, trackinfofile);
                strcat (buffer, " &");
                system (buffer);
                fclose (f);

                /* stampo la traccia avviata in info_win */
                wmove (info_win, 2, 2);
                wclrtoeol (info_win);
                box (info_win, 0, 0);
                wattron (info_win, A_BLINK);
                wprintw (info_win, "- ");
                wattroff (info_win, A_BLINK);
                wprintw (info_win, "Playing: %s", *(songs + highlight - 1));
                wrefresh (info_win);

                break;

            case KEY_RIGHT:
            case KEY_LEFT:
            case '\t':
                free (songs);
                return ;
                break;

            default:
               break;
    }

    print_menu (menu_win, highlight, songs, n_tracks, HEIGHT, WIDTH);
    wrefresh (menu_win);
    
    for (c = 0; c < 7; c++)
        mvwprintw (info_win, 1, 3 + c, " ");
    mvwprintw (info_win, 1, 2, "%d/%d", highlight, n_tracks);
    wrefresh (info_win);

    }


}

void addSongtoPlaylist (void)
{
    char **m;
    char **inputSongs;
    char **fileSongs;
    char **mergedSongs;

    int *v;
    int highlight;
    int bottom, top;
    int playlistnum;
    int fLines;
    int mergedLen;
    int i, j, c, k;
    int x, y;
    int out;

    FILE *f;
    FILE *fIn;


    x = 2;
    y = 1;

    /* lista le playlist nella cartella */
    strcpy (buffer, "ls *.ply >& ");
    strcat (buffer, playlistfile);
    system (buffer);

    /* caso in cui non esistano le playlist */
    f = fopen (playlistfile, "r");
    for (i = 0; (c = fgetc (f)) != '\n'; i++)
        buffer[i] = c;
    buffer[i] = '\0';

    if (strstr (buffer, "ls: "))
    {
        strcpy (buffer, "rm ");
        strcat (buffer, playlistfile);
        system (buffer);
        fclose (f);
        mvprintw (starty - 1, startx, "Nessuna playlist a cui aggiungere");
        refresh ();
        return ;
    }

    rewind (f);
    for (fLines = 0; !feof (f) ; )
        if ((c = fgetc (f)) == '\n')
            fLines++;
    
    m = malloc (sizeof (char *) * (fLines + 1));
    //*(m  + fLines + 1) = NULL;
    rewind (f);

    for (j = i = 0; 1 /*j < fLines */; i++)
    {
        c = fgetc (f);

        if (c != '\n' && c != EOF)
            buffer[i] = c;
        else
        {
            buffer[i] = '\0';

            /* alloco il vettore e copio il nome della traccia */
            *(m + j) = malloc (sizeof (char) * (i + 1));
            strcpy (*(m + j), buffer);

            /* fine file */
            if (c == EOF)
                break;

            refresh ();
            j++;
            i = -1;
        }
    }
    *(m + j) = '\0';
    fclose (f);


    werase (menu_win);
    wmove (menu_win, y, x),
    wattron (menu_win, A_BOLD);
    wprintw (menu_win, "Scegliere la playlist");
    wattroff (menu_win, A_BOLD);
    wrefresh (menu_win);

    box (menu_win, 0, 0);
    wrefresh (menu_win);

    for (out = 0, highlight = 1; !out; )
    {
        switch (c = wgetch (menu_win))
        {
            case KEY_UP:
                if (highlight == 1)
                    highlight = fLines;
                else
                    --highlight;
                break;

            case KEY_DOWN:
                if (highlight == fLines)
                    highlight = 1;
                else
                    ++highlight;
                break;

            case '\t':
                return ;
                break;

            case '\n':
                out = 1;
                playlistnum = highlight;
                break;


            default:
                break;
        }

        print_menu (menu_win, highlight, m, fLines, HEIGHT, WIDTH);
        wrefresh (menu_win);
    }

    wmove (menu_win, 1, 2);
    wclrtoeol (menu_win);
    wattron (menu_win, A_BOLD);
    wprintw (menu_win, "Selezionare le tracce\t    <F1> abortire <TAB> per uscire");
    wattroff (menu_win, A_BOLD);
    wrefresh (menu_win);

    v = malloc (sizeof (int) * (n_choices));
    for (i = 0; i < n_choices; i++)
        v[i] = 0;

    for (out = 0, highlight = 1; !out; )
    {
        switch (c = wgetch (menu_win))
        {

            case KEY_UP:
                if (highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;

            case KEY_DOWN:
                if (highlight == n_choices)
                    highlight = 1;
                else
                    ++highlight;
                break;

            case KEY_F(1):
                move (starty - 1, startx);
                printw ("Aborting from addSongToPlaylist ()");
                refresh ();
                return ;
             
            case '\t':
                out = 1;
                break;

            case '\n':
                if (!v[highlight - 1])
                    v[highlight - 1] = 1;
                else
                    v[highlight - 1] = 0;
                break;

            default:
                break;
        }
    
        print_menu (menu_win, highlight, choises, n_choices, HEIGHT, WIDTH);
        wrefresh (menu_win);

        y = starty + 2;
        i = highlight - 1;

        if (i < (n_choices - HEIGHT + 4) && n_choices > HEIGHT)
        {
            for (k = 0; k < n_choices && k < HEIGHT - 4; i++, k++, y++)
            {
                if (v[i])
                {

                    wattron (menu_win, A_BLINK);
                    mvwprintw (menu_win, k + 2, 1, "-");
                    wattroff (menu_win, A_BLINK);
                }
                else
                    mvwprintw (menu_win, k + 2, 1, " ");
            }
            refresh ();
        }
        else
        {
            top = n_choices - highlight;
            bottom = HEIGHT - 5 - top;

            for (j = highlight - bottom, k = 0; k < bottom; k++, j++)
            {
                if (v[j - 1])
                {
                    wattron (menu_win, A_BLINK);
                    mvwprintw (menu_win, k + 2, 1, "-");
                    wattroff (menu_win, A_BLINK);
                }
                else
                    mvwprintw (menu_win, k + 2, 1, " ");
            }

            if (v[j - 1])
            {
                wattron (menu_win, A_BLINK);
                mvwprintw (menu_win, k + 2, 1, "-");
                wattroff (menu_win, A_BLINK);
            }
            else
                mvwprintw (menu_win, k + 2, 1, " ");

            for (i = 0, k++, j++; i < top; i++, k++, j++)
            {
                if (v[j - 1])
                {
                    wattron (menu_win, A_BLINK);
                    mvwprintw (menu_win, k + 2, 1, "-");
                    wattroff (menu_win, A_BLINK);
                }
                else
                    mvwprintw (menu_win, k + 2, 1, " ");
            }
            wrefresh (menu_win);
        }
    }

    /* pulisco le selezioni */
    for (i = 0; i < HEIGHT; i++)
        mvwprintw (menu_win, i + 2, 1, " ");

    fIn = fopen (playlists[playlistnum - 1], "r");

    for (i = 0; (c = fgetc (fIn)) != EOF ; )
        if (c == '\n')
            i++;

    move (2, 0);
    /* copiatura delle tracce nella playlist */
    fileSongs = malloc (sizeof (char *) * (i + 1));
    for (rewind (fIn), j = k = 0; k < i; j++)
    {
            c = fgetc (f);

            if (c == EOF)
                break;
            else if (c == '\n')
            {
                buffer[j] = '\0';

                *(fileSongs + k) = malloc (sizeof (char) * j);
                strcpy (*(fileSongs + k), buffer);
                j = -1;
                k++;
            }
            else
                buffer[j] = c;
    }
    *(fileSongs + k) = NULL;

    /* lunghezza della matrice */
    for (i = j = 0; i < n_choices; i++)
        if (v[i])
            j++;
    
    inputSongs = malloc (sizeof (char *) * (j + 1));

    for (i = j = 0; i < n_choices; i++)
        if (v[i])
        {
            *(inputSongs + j) = malloc (sizeof (char) * (strlen (choises[i]) + 1));
            strcpy (*(inputSongs + j), choises[i]);
            j++;
        }
    *(inputSongs + j) = NULL;

    mergedLen = j + k + 1;
    mergedSongs = malloc (sizeof (char *) * mergedLen);

    for (i = j = k = 0; ;)
    {
        if (*(fileSongs + j) == NULL)
        {
            for (; *(inputSongs + k) != NULL; k++, i++)
            {
                if (i && !strcmp (*(mergedSongs + i - 1), *(inputSongs + k)))
                {
                    i--;
                    k++;
                }
                else
                {
                    *(mergedSongs + i) = malloc (sizeof (char) * (strlen (*(inputSongs + k)) + 1));
                    strcpy (*(mergedSongs + i), *(inputSongs + k));
                }
            }
            break;
        }
        else if (*(inputSongs + k) == NULL)
        {
            for (; *(fileSongs + j) != NULL; j++, i++)
            {
                if (i && !strcmp (*(mergedSongs + i - 1), *(fileSongs + j)))
                {
                    i--;
                    j++;
                }
                else
                {
                    *(mergedSongs + i) = malloc (sizeof (char) * (strlen (*(fileSongs + j)) + 1));
                    strcpy (*(mergedSongs + i), *(fileSongs + j));
                }
            }
            break;    
        }
        else if (!strcmp (*(fileSongs + j), *(inputSongs + k)))
        {
            *(mergedSongs + i) = malloc (sizeof (char) * (strlen (*(fileSongs + j)) + 1));
            strcpy (*(mergedSongs + i), *(fileSongs + j));
            j++;
            k++;
            i++;
            mergedLen--;
        }

        else if (strcmp (*(fileSongs + j), *(inputSongs + k)) > 0)
        {
            *(mergedSongs + i) = malloc (sizeof (char) * (strlen (*(inputSongs + k)) + 1));
            strcpy (*(mergedSongs + i), *(inputSongs + k));
            k++;
            i++;
        }
        else //if (strcmp (*(fileSongs + j), *(inputSongs + k)) < 0)
        {
            *(mergedSongs + i) = malloc (sizeof (char) * (strlen (*(fileSongs + j)) + 1 ));
            strcpy (*(mergedSongs + i), *(fileSongs + j));
            j++;
            i++;
        }
        refresh ();
    }
    *(mergedSongs + mergedLen - 1) = NULL;
    fclose (fIn);

    fIn = fopen (playlists[playlistnum - 1], "w");
    for (i = 0; *(mergedSongs + i) != NULL ; i++)
        fprintf (fIn, "%s\n", *(mergedSongs + i));
    fclose (fIn);
    
    free (mergedSongs);
    free (inputSongs);
    free (fileSongs);
    free (v);
    free (m);
    

    return ;
}
