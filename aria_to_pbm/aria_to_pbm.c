/*
 * Copyright (c) 2012, Kevin LeBlanc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */




//=============================================================================
//Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>




//=============================================================================
//Defines
//=============================================================================
#define LINES_START_STRING      "LINES\n"
#define RESOLUTION_MULTIPLIER   0.1
#define LINE_THRESHOLD          40
#define MAX(x,y)                (((x) > (y)) ? (x) : (y))
#define MIN(x,y)                (((x) < (y)) ? (x) : (y))
#define ABS(x)                  (((x) < 0) ? -(x) : (x))




//=============================================================================
//Types
//=============================================================================
typedef struct {
  int x1;
  int x2;
  int y1;
  int y2;
} Line;




//=============================================================================
//Main
//=============================================================================
/**
 * This program opens and reads a given ARIA *.map file, and creates a
 * corresponding *.pbm file containing the same lines.
 */
int main(int argc, char* argv[]) {
  int i, x, y;
  FILE* fileAria = NULL;
  FILE* fileImage = NULL;
  char filenameAria[PATH_MAX] = "";
  char filenameImage[PATH_MAX] = "";
  char string[LINE_MAX] = "";
  int xmin = INT_MAX;
  int ymin = INT_MAX;
  int xmax = 0;
  int ymax = 0;
  Line* lines = NULL;
  int numLines = 0;
  int usageRequested = 0;

  //Check for usage request
  for (i = 1; i < argc; i++)
    if ((0 == strcmp(argv[i], "-h")) || (0 == strcmp(argv[i], "--help")))
      usageRequested = 1;

  //Check for usage request or invalid number of arguments
  if ((0 != usageRequested) || (3 != argc)) {
    printf("Usage: %s <filenameAria.map> <filenameImage.ppm>\n", argv[0]);
    return 1;
  }

  //Set filenames
  strncpy(filenameAria, argv[1], PATH_MAX);
  strncpy(filenameImage, argv[2], PATH_MAX);

  //Open files
  fileAria = fopen(filenameAria, "r");
  if (fileAria == NULL) {
    printf("Failed to open aria file (%s)\n", filenameAria);
    return 1;
  }
  fileImage = fopen(filenameImage, "w");
  if (fileImage == NULL) {
    printf("Failed to open image file (%s)\n", filenameImage);
    return 1;
  }

  //Read until start of lines
  while(1) {
    if (NULL == fgets(string, LINE_MAX, fileAria)) {
      printf("Failed to detect lines section in aria file (%s)\n", filenameAria);
      return 1;
    }
    if (0 == strcmp(LINES_START_STRING, string)) {
      break;
    }
  }

  //Read lines
  while(1) {
    Line* newLines;

    //Read one line (stop at EOF or when first character is non-numeric)
    if (NULL == fgets(string, LINE_MAX, fileAria))
      break;
    if (string[0] < '0' || string[0] > '9')
      break;

    //Update
    //printf("%s", string);

    //Allocate space for this line
    newLines = (Line*)realloc(lines, sizeof(Line)*++numLines);
    if (NULL == newLines) {
      printf("Failed to allocate memory\n");
      if (NULL != lines) {
        free(lines);
      }
      return 1;
    }
    lines = newLines;

    //Scan this line
    sscanf(string, "%d %d %d %d",
      &(lines[numLines-1].x1),
      &(lines[numLines-1].y1),
      &(lines[numLines-1].x2),
      &(lines[numLines-1].y2));

    //Update min and max values
    if (lines[numLines-1].x1 < xmin) xmin = lines[numLines-1].x1;
    if (lines[numLines-1].x2 < xmin) xmin = lines[numLines-1].x2;
    if (lines[numLines-1].x1 > xmax) xmax = lines[numLines-1].x1;
    if (lines[numLines-1].x2 > xmax) xmax = lines[numLines-1].x2;
    if (lines[numLines-1].y1 < ymin) ymin = lines[numLines-1].y1;
    if (lines[numLines-1].y2 < ymin) ymin = lines[numLines-1].y2;
    if (lines[numLines-1].y1 > ymax) ymax = lines[numLines-1].y1;
    if (lines[numLines-1].y2 > ymax) ymax = lines[numLines-1].y2;
  }

  //Sanity check
  if (0 == numLines) {
    printf("No lines found\n");
    if (NULL != lines) {
      free(lines);
    }
    return 1;
  }

  //Update
  //printf("After reading %d lines, the min/max points are: (%d,%d) -> (%d, %d)\n", numLines, xmin, ymin, xmax, ymax);

  //Convert lines from ARIA coordinate system to image coordinate system
  for (i = 0; i < numLines; i++) {
    //Shift so that minimum value is 0
    lines[i].x1 -= xmin;
    lines[i].x2 -= xmin;
    lines[i].y1 -= ymin;
    lines[i].y2 -= ymin;

    //Flip so that the top of the vertical axis is 0
    lines[i].y1 = ymax - lines[i].y1;
    lines[i].y2 = ymax - lines[i].y2;

    //Update resolution
    lines[i].x1 *= RESOLUTION_MULTIPLIER;
    lines[i].x2 *= RESOLUTION_MULTIPLIER;
    lines[i].y1 *= RESOLUTION_MULTIPLIER;
    lines[i].y2 *= RESOLUTION_MULTIPLIER;

    //Update
    //printf("%d %d %d %d\n", lines[i].x1, lines[i].y1, lines[i].x2, lines[i].y2);
  }

  //Convert min and max values from ARIA coordinate system to image coordinate system
  xmax -= xmin;
  ymax -= ymin;
  xmin = 0;
  xmin = 0;
  xmax *= RESOLUTION_MULTIPLIER;
  ymax *= RESOLUTION_MULTIPLIER;

  //Update
  //printf("After conversions, the min/max points are: (%d,%d) -> (%d, %d)\n", xmin, ymin, xmax, ymax);

  //Write image file header
  fprintf(fileImage, "P1\n#\n%d %d\n", xmax + 1, ymax + 1);

  //Draw each pixel
  for (y = 0; y < ymax + 1; y++) {
    for (x = 0; x < xmax + 1; x++) {
      int onLine = 0;

      //For each line, check if point is on line
      for (i = 0; i < numLines; i++) {
        int diff;

        //If the line has length zero ignore it
        if ((lines[i].x1 == lines[i].x2) && (lines[i].y1 == lines[i].y2))
          continue;

        //If the point is before or after the line ignore it
        if ((MIN(lines[i].x1, lines[i].x2) > x) ||
            (MAX(lines[i].x1, lines[i].x2) < x) ||
            (MIN(lines[i].y1, lines[i].y2) > y) ||
            (MAX(lines[i].y1, lines[i].y2) < y))
          continue;

        //The point (x, y) is on line lines[i] iff (x-x1)*(y2-y1)==(x2-x1)*(y-y1)
        diff = ABS(
          (ABS(x           - lines[i].x1))*(ABS(lines[i].y2 - lines[i].y1)) -
          (ABS(lines[i].x2 - lines[i].x1))*(ABS(y           - lines[i].y1))
        );

        //Include points close to the line to avoid errors due to dithering and rounding
        if (diff <= LINE_THRESHOLD) {
          onLine = 1;
          break;
        }
      }

      //If point is not on line write a 0, otherwise write a 1
      if (0 == onLine) {
        fprintf(fileImage, "%d ", 0);
      } else {
        fprintf(fileImage, "%d ", 1);
      }
    }
    fprintf(fileImage, "\n");
  }

  //Free lines
  if (NULL != lines) {
    free(lines);
  }

  //Close files
  fclose(fileAria);
  fclose(fileImage);

  //Done
  return 0;
}
