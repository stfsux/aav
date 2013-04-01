/* gcc -o aav aav.c -std=c99 -Wall -Wextra */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <wchar.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define FILE_UNK    (unsigned char)(~0)
#define FILE_ANSI   1
#define FILE_ASCII  2
#define FILE_BIN    3

#define AAV_USAGE L"aav [OPTIONS] FILE...\n"

/* Code pages 437 convertion to Unicode charset.         */
/* 0-127 ASCII compilant which dont need any convertion. */
/* 128-255 begin of the bullshit.                        */
wchar_t cp437[] = 
{
  L'\x00C7',L'\x00FC',L'\x00E9',L'\x00E2',
  L'\x00E4',L'\x00E0',L'\x00E5',L'\x00E7',
  L'\x00EA',L'\x00EB',L'\x00E8',L'\x00EF',
  L'\x00EE',L'\x00EC',L'\x00C4',L'\x00C5',

  L'\x00C9',L'\x00E6',L'\x00C6',L'\x00F4',
  L'\x00F6',L'\x00F2',L'\x00FB',L'\x00F9',
  L'\x00FF',L'\x00D6',L'\x00DC',L'\x00A2',
  L'\x00A3',L'\x00A5',L'\x20A7',L'\x0192',

  L'\x00E1',L'\x00ED',L'\x00F3',L'\x00FA',
  L'\x00F1',L'\x00D1',L'\x00AA',L'\x00BA',
  L'\x00BF',L'\x2310',L'\x00AC',L'\x00BD',
  L'\x00BC',L'\x00A1',L'\x00AB',L'\x00BB',

  L'\x2591',L'\x2592',L'\x2593',L'\x2502',
  L'\x2524',L'\x2561',L'\x2562',L'\x2556',
  L'\x2555',L'\x2563',L'\x2551',L'\x2557',
  L'\x255D',L'\x255C',L'\x255B',L'\x2510',

  L'\x2514',L'\x2534',L'\x252C',L'\x251C',
  L'\x2500',L'\x253C',L'\x255E',L'\x255F',
  L'\x255A',L'\x2554',L'\x2569',L'\x2566',
  L'\x2560',L'\x2550',L'\x256C',L'\x2567',

  L'\x2568',L'\x2564',L'\x2565',L'\x2559',
  L'\x2558',L'\x2552',L'\x2553',L'\x256B',
  L'\x256A',L'\x2518',L'\x250C',L'\x2588',
  L'\x2584',L'\x258C',L'\x2590',L'\x2580',

  L'\x03B1',L'\x00DF',L'\x0393',L'\x03C0',
  L'\x03A3',L'\x03C3',L'\x00B5',L'\x03C4',
  L'\x03A6',L'\x0398',L'\x03A9',L'\x03B4',
  L'\x221E',L'\x03C6',L'\x03B5',L'\x2229',

  L'\x2261',L'\x00B1',L'\x2265',L'\x2264',
  L'\x2320',L'\x2321',L'\x00F7',L'\x2248',
  L'\x00B0',L'\x2219',L'\x00B7',L'\x221A',
  L'\x207F',L'\x00B2',L'\x25A0',L'\x00A0',
};

char* strtolower (char* str);
unsigned char get_file_ext (char* str);
unsigned char is_csi_endcmd (char c);
void draw_ansi (char* ansi, unsigned int size);
void draw_ascii (char* ascii, unsigned int size);

int
 main (int argc, char* const argv[])
{
  char* locale = setlocale (LC_ALL, "");
  int infd = 0;
  unsigned int insize = 0;
  void* inmap = NULL;
  unsigned char ft = 0;

  locale = strtolower (locale);
  if (strstr (locale, "utf8") == NULL && 
      strstr (locale, "utf-8") == NULL)
  {
    fprintf (stderr, "%s: you must have utf-8 encoding charset.\n", argv[0]);
    return 0;
  }
  if (argc < 2)
  {
    fwprintf (stderr, AAV_USAGE);
    return 0;
  }
  infd = open (argv[1], O_RDONLY);
  if (infd == -1)
  {
    fwprintf (stderr, L"%s: cannot open file `%s' for reading.\n", argv[0], argv[1]);
    return 0;
  }
  insize = lseek (infd, 0, SEEK_END);
  lseek (infd, 0, SEEK_SET);
  inmap = mmap (NULL, insize, PROT_READ, MAP_SHARED, infd, 0);
  if (inmap == (void*)(-1))
  {
    close (infd);
    fwprintf (stderr, L"%s: cannot map file `%s' into memory.\n", argv[0], argv[1]);
    return 0;
  }
  ft = get_file_ext (argv[1]);
  if (ft == (unsigned char)~0)
  {
    fwprintf (stderr, L"%s: cannot identify file type of `%s'.\n", argv[0], argv[1]);
    munmap (inmap, insize);
    close (infd);
    return 0;
  }
  switch (ft)
  {
    case FILE_ANSI:
      draw_ansi (inmap, insize);
      break;

    case FILE_ASCII:
      draw_ascii (inmap, insize);
      break;

    default:
      fwprintf (stderr, L"%s: file type not supported.\n", argv[0]);
      break;
  }
  munmap (inmap, insize);
  close (infd);
  return 0;
}

char*
 strtolower (char* str)
{
  unsigned int i = 0;
  for (i = 0; i < strlen (str); i++)
    str[i] = tolower(str[i]);
  return str;
}

unsigned char
 get_file_ext (char* str)
{
  char ext[10];
  unsigned int i = 0;
  char* ptr = strrchr (str, '.');
  if (ptr == NULL)
    return ~0;
  if (strlen (ptr) > 9)
    return ~0;
  for (i = 0; i < strlen(ptr); i++)
    ext[i] = tolower(ptr[i]);
  ext[i] = 0;
  if (!strcmp (ext, ".ans"))
    return FILE_ANSI;
  else if (!strcmp (ext, ".asc") ||
           !strcmp (ext, ".nfo") ||
           !strcmp (ext, ".diz"))
    return FILE_ASCII;
  else if (!strcmp (ext, ".bin"))
    return FILE_BIN;
  return ~0;
}

unsigned char
 is_csi_endcmd (char c)
{
  char csi[] = 
  {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J',
    'K', 'S', 'T', 'f', 'm', 'n', 's', 'u', 'l',
    'h'
  };
  unsigned int i;
  for (i = 0; i < strlen (csi); i++)
  {
    if (csi[i] == c)
      return 1;
  }
  return 0;
}

/* Looks like a generated FSM code, isn't it? */
/* But I really wrote that shit...            */
void
 draw_ansi (char* ansi, unsigned int size)
{
  char* ptr = ansi;
  char* endptr = (char*)((unsigned long)ansi+size);
  unsigned char state = 0;
  unsigned int npc = 0, nargs = 0, n = 0, npcs = 0;
  unsigned int args[10];
  char cmd[10];
  unsigned char parse_end = 0;

  while (ptr != endptr && parse_end == 0)
  {
_nextstate:
    switch (state)
    {
      case 0:
        if (*ptr == 0x1B)
        {
          fputwc (btowc (*ptr), stdout);
          state = 1;
        }
        else if (!strncmp (ptr, "\x1ASAUCE00", 7))
        {
          parse_end = 1;
          break;
        }
        else
        {
          if ((unsigned char)*ptr >= (unsigned char)128)
            fputwc (cp437[(*ptr-128)&0xFF], stdout);
          else
            fputwc (btowc(*ptr), stdout);
          if (npc < 79)
            npc++;
          else
          {
            if (ptr+1 < endptr)
            {
              if (*(ptr+1) != '\n')
                wprintf ( L"\e[0m\n");
              npc = 0;
            }
          }
          if (*ptr == '\n' || *ptr == '\r')
            npc = 0;
        }
        goto _nextchar;

      case 1:
        fputwc (btowc (*ptr), stdout);
        if (*ptr == '[') state = 2;
        else state = 1;
        goto _nextchar;

      case 2:
        if (is_csi_endcmd (*ptr) ||
            *ptr == ';')
        {
          state = 3;
          goto _nextstate;
        }
        else cmd[n++] = *ptr;
        goto _nextchar;

      case 3:
        cmd[n] = 0;
        n = 0;
        args[nargs] = strtol (cmd, NULL, 10);
        if (args[nargs] == 40)
          args[nargs] = 49;
        switch (*ptr)
        {
          case 'E':
          case 'F':
            npc = 0;
            wprintf ( L"\e[0m\n");
            break;

          case 'G':
            npc = args[0]-1;
            break;

          case 'H':
          case 'f':
            npc = args[1]-1;
            break;

          case 'C':
            npc = npc+args[0];
            if (npc >= 79)
            {
              wprintf ( L"\e[0m\n");
              npc = npc%80;
            }
            break;

          case 'D':
            if (npc != 0)
              npc = npc - args[0];
            break;

          case 's':
            npcs = npc;
            break;

          case 'u':
            npc = npcs;
            break;
        }
        wprintf (L"%u%c", args[nargs], *ptr);
        nargs++;
        if (is_csi_endcmd (*ptr))
        {
          n = 0;
          nargs = 0;
          state = 0;
        }
        else state = 2;
        goto _nextchar;
    }
_nextchar:
    ptr++;
  }
  wprintf (L"\e[m\n");
}

void
 draw_ascii (char* ascii, unsigned int size)
{
  char* ptr = ascii;
  char* endptr = (char*)((unsigned long)ascii+size);
  while (ptr != endptr)
  {
    if ((unsigned char)*ptr >= 128)
      fputwc (cp437[(*ptr-128)&0xFF], stdout);
    else
      fputwc (btowc(*ptr), stdout);
    ptr++;
  }
}

