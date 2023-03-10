/*
 repcopyin: Copy a file into the repository.
 SPDX-FileCopyrightText: © 2007-2011 Hewlett-Packard Development Company, L.P.

 SPDX-License-Identifier: LGPL-2.1-only
*/
/**
 * \file
 * \brief Copy a file into the repository.
 *
 * Input can be from stdin or command-line.
 * stdin can be pairs of files, or xml.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "libfossrepo.h"

#ifdef COMMIT_HASH
char BuildVersion[]="Build version: " COMMIT_HASH ".\n";
#endif

/*** GLOBALS (for stats) ***/
long TotalImported = 0;
long TotalDuplicate = 0;
long TotalError = 0;


/**
 * Given one file, copy it.
 * @param Source  Source file path
 * @param Type    Type of source
 * @param Name    Destination file name
 * @sa fo_RepImport()
 */
void CopyFile(char* Source, char* Type, char* Name)
{
  int rc;
  rc = fo_RepExist(Type, Name);
  if (rc == 1)
  {
    TotalDuplicate++;
    return;
  }
  else if (rc == -1)
  {
    TotalError++;
    return;
  }
  if (fo_RepImport(Source, Type, Name, 1) == 0) TotalImported++;
  else TotalError++;
} /* CopyFile() */

/**
 * Read a line from stdin.
 * @param Fin       File to read from (STDIN)
 * @param[out] Line Line read
 * @param MaxLine   Max length of line
 * @return Number of bytes loaded.
 */
int ReadLine(FILE* Fin, char* Line, int MaxLine)
{
  int C = '@';
  int i = 0;      /* index */
  memset(Line, 0, MaxLine);
  if (feof(Fin)) return (-1);
  while (!feof(Fin) && (i < MaxLine - 1) && (C != '\n') && (C > 0))
  {
    C = fgetc(Fin);
    if ((C > 0) && (C != '\n'))
    {
      Line[i] = C;
      i++;
    }
  }
  return (i);
} /* ReadLine() */

/**
 * Given a hex character, return decimal value.
 * @param C Hex character to convert
 * @return Decimal value of C
 */
int Hex2Dec(char C)
{
  if (!isxdigit(C)) return (0);
  if (isdigit(C)) return (C - '0');
  if (isupper(C)) return (C - 'A' + 10);
  return (C - 'a' + 10);
} /* Hex2Dec() */

/**
 * Convert "&#xAA;" to hex.
 * @param S Unicode character
 * @return Returns hex or -1 on error.
 */
int UnUnicodeHex(char* S)
{
  int v;
  if ((S[0] == '&') && (S[1] == '#') && (S[2] == 'x')
    && isxdigit(S[3]) && isxdigit(S[4]) && (S[5] == ';'))
  {
    v = Hex2Dec(S[3]) * 16 + Hex2Dec(S[4]);
    return (v);
  }
  return (-1);
} /* UnUnicodeHex() */

/**
 * Process pairs of names in format: `dest src`
 * @param Fin   FP to read from
 * @param Type  Type of file
 */
void ProcessPairs(FILE* Fin, char* Type)
{
  char Buf[10240];
  char Dst[FILENAME_MAX];
  char Src[FILENAME_MAX];
  int Space;
  int i, s, c;

  while (ReadLine(Fin, Buf, sizeof(Buf)) > 0)
  {
    Space = 0;
    /* save the dst name */
    while ((Buf[Space] != '\0') && (Buf[Space] != ' ')) Space++;
    strncpy(Dst, Buf, Space);
    /* skip the space */
    /* save the src name and remove unicode stuff */
    memset(Dst, '\0', sizeof(Dst));
    memset(Src, '\0', sizeof(Src));
    strncpy(Dst, Buf, Space);
    s = 0;
    for (i = Space + 1; Buf[i] != '\0'; i++)
    {
      if (Buf[i] != '&') Src[s++] = Buf[i];
      else
      {
        c = UnUnicodeHex(Buf + i);
        if (c >= 0)
        {
          Src[s++] = c;
          i += 5;
        }
        else Src[s++] = Buf[i];
      }
    }
#if 0
    printf("Dst='%s'  Src='%s'\n",Dst,Src);
#endif
    CopyFile(Src, Type, Dst);
  }
} /* ProcessPairs() */

/**
 * @brief Process Ununpack XML.
 *
 * Format comes from Ununpack -L.
 * We care about fuid and source fields.
 * We want to skip directories and artifacts.
 * @param Fin         File pointer
 * @param TypeSource  Type of source
 * @param Type        Type of destination
 */
void ProcessXML(FILE* Fin, char* TypeSource, char* Type)
{
  char Buf[10240];
  char Dst[FILENAME_MAX];
  char Src[FILENAME_MAX];
  int i, j, c;
  char* fuid;
  char* source;
  char* field;
  char* FileType;

  /* From ununpack: each line is either an item or a /item. */
  while (ReadLine(Fin, Buf, sizeof(Buf)) > 0)
  {
    /* only process known item types */
    if (!strncmp(Buf, "<item ", 6)) FileType = Type;
    else if (!strncmp(Buf, "<source ", 8)) FileType = TypeSource;
    else continue; /* skip this type */

    /* skip items without FUID values */
    fuid = strstr(Buf, " fuid=\"");
    if (!fuid) continue;
    fuid += 7; /* jump to start of string; string ends with quote */

    /* find the source */
    source = strstr(Buf, " source=\"");
    if (!source) continue;
    source += 9; /* jump to start of string; string ends with quote */

    /* skip artifacts and directories */
    field = strstr(Buf, " artifact=\"1\"");
    if (field) continue;
    field = strstr(Buf, " mime=\"directory\"");
    if (field) continue;

    /* save the src name and remove unicode stuff */
    memset(Dst, '\0', sizeof(Dst));
    memset(Src, '\0', sizeof(Src));
    j = 0;
    for (i = 0; source[i] != '\"'; i++)
    {
      if (source[i] != '&') Src[j++] = source[i];
      else
      {
        c = UnUnicodeHex(source + i);
        if (c >= 0)
        {
          Src[j++] = c;
          i += 5;
        }
        else Src[j++] = source[i];
      }
    }
    j = 0;
    for (i = 0; fuid[i] != '\"'; i++)
    {
      if (fuid[i] != '&') Dst[j++] = fuid[i];
      else
      {
        c = UnUnicodeHex(fuid + i);
        if (c >= 0)
        {
          Dst[j++] = c;
          i += 5;
        }
        else Dst[j++] = fuid[i];
      }
    }
#if 0
    printf("Dst='%s'  Src='%s'\n",Dst,Src);
#endif
    CopyFile(Src, FileType, Dst);
    /* always make sure a copy is put in the regular file repository */
    if (FileType != Type) CopyFile(Src, Type, Dst);
  }
} /* ProcessXML() */

/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
int main(int argc, char* argv[])
{
  if ((argc < 2) || (argc > 4))
  {
    fprintf(stderr, "Usage: (depends on the parameters)\n");
    fprintf(stderr, "   %s type filename sha1.md5.len\n", argv[0]);
    fprintf(stderr, "   echo 'sha1.md5.len filename' | %s type\n", argv[0]);
    fprintf(stderr, "   echo '<xml from ununpack>...</xml>' | %s typesource type\n", argv[0]);
    fprintf(stderr, "     type = repository for storing files\n");
    fprintf(stderr, "     typesource = repository for storing source files (XML only)\n");
    exit(-1);
  }

  switch (argc)
  {
    case 2:        /* pairs from stdin */
      ProcessPairs(stdin, argv[1]);
      break;
    case 3:        /* pairs from XML */
      ProcessXML(stdin, argv[1], argv[2]);
      break;
    case 4:        /* pairs from command-line */
      CopyFile(argv[2], argv[1], argv[3]);
      break;
  }

  printf("Total Imported:   %ld\n", TotalImported);
  printf("Total Duplicates: %ld\n", TotalDuplicate);
  printf("Total Errors:     %ld\n", TotalError);
  if (TotalError > 0) return (1);
  return (0);
} /* main() */

