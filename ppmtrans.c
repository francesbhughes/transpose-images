/*
 * Names: Frances Hughes
 * Created: 2/20/2017
 *
 * ppmtrans.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "except.h"
#include "assert.h"
#include "pnm.h"
#include "cputiming.h"

Except_T fileE;

#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (0)

/*##########################FUNCTION DECLARATIONS###########################*/
static int rotate(int rotation_degree, FILE *fp, A2Methods_T methods, 
                A2Methods_mapfun *map, char *time_file_name);
void ninety(int i, int j, A2Methods_UArray2 array, void *elem, void *pFile);
void one_eighty(int i, int j, A2Methods_UArray2 array, void *elem, void *pFile);

/*
 * Prints error message if input is in wrong format
 */
static void usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] "
                        "[-{row,col,block}-major] [filename]\n",
                        progname);
        exit(1);
}

int main(int argc, char *argv[]) 
{
        char *time_file_name = NULL;
        int   rotation       = 0;
        int   i;

        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain; 
        assert(methods);

        /* default to best map */
        A2Methods_mapfun *map = methods->map_default; 
        assert(map);
   
        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-row-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_row_major, 
                                        "row-major");
                } else if (strcmp(argv[i], "-col-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_col_major, 
                                        "column-major");
                } else if (strcmp(argv[i], "-block-major") == 0) {
                        SET_METHODS(uarray2_methods_blocked, map_block_major,
                                        "block-major");
                } else if (strcmp(argv[i], "-rotate") == 0) {
                        if (!(i + 1 < argc)) {      /* no rotate value */
                                usage(argv[0]);
                        }
                        char *endptr;
                        rotation = strtol(argv[++i], &endptr, 10);
                        if (!(rotation == 0 || rotation == 90 ||
                                        rotation == 180 || rotation == 270)) {
                                fprintf(stderr, 
                                        "Rotation must be 0, 90 180 or 270\n");
                                usage(argv[0]);
                        }
                        if (!(*endptr == '\0')) {    /* Not a number */
                                usage(argv[0]);
                        }
                } else if (strcmp(argv[i], "-time") == 0) {
                        time_file_name = argv[++i];
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n", argv[0],
                                        argv[i]);
                } else if (argc - i > 1) {
                        fprintf(stderr, "Too many arguments\n");
                        usage(argv[0]);
                } else {
                        break;
                } 
        }
        int ret_target;
        if (argc == i) {
                if (!feof(stdin)) {
                        ret_target = rotate(rotation, stdin, methods, map, 
                                        time_file_name);
                        return ret_target;
                }
        } else {
                FILE *fp = fopen(argv[argc-1], "rb");
                if (fp == NULL) {
                        fprintf(stderr, "unable to open file\n");
                        exit(EXIT_FAILURE);
                }
                ret_target = rotate(rotation, fp, methods, map, time_file_name);
                fclose(fp);
                return ret_target;
        }
        return EXIT_SUCCESS;
}

/*
 * Determines to what degree the image should be rotated, rotates the image, 
 * writes the image, and calculates total time it takes to rotate the image
 */
static int rotate(int rotation_degree, FILE *fp, A2Methods_T methods, 
                A2Methods_mapfun *map, char *time_file_name)
{            
        Pnm_ppm pixFile = Pnm_ppmread(fp, methods);
        
        /*Timer created */
        CPUTime_T timer = CPUTime_New();
        double time_used;
        double average;
        if (rotation_degree == 0){
                CPUTime_Start(timer);
                Pnm_ppmwrite(stdout, pixFile);
                time_used = CPUTime_Stop(timer);
        } else {
                /* New Pnm_ppm for rotated image */
                Pnm_ppm newPix = malloc(sizeof(struct Pnm_ppm));
                newPix->denominator = pixFile->denominator;
                newPix->methods = pixFile->methods;
                newPix->width = pixFile->width;
                newPix->height = pixFile->height;
                if (rotation_degree == 90){ 
                        newPix->width = pixFile->height;
                        newPix->height = pixFile->width;
                        newPix->pixels = methods->new(pixFile->height, 
                                pixFile->width, sizeof(struct Pnm_rgb));
                        CPUTime_Start(timer);
                        map(pixFile->pixels,ninety,&newPix);
                        time_used = CPUTime_Stop(timer);
                        Pnm_ppmwrite(stdout, newPix);
                        Pnm_ppmfree(&newPix);
                } else if (rotation_degree == 180){
                        newPix->pixels = methods->new(pixFile->width, 
                                pixFile->height, sizeof(struct Pnm_rgb));
                        CPUTime_Start(timer);
                        map(newPix->pixels,one_eighty,&pixFile);
                        time_used = CPUTime_Stop(timer);
                        Pnm_ppmwrite(stdout, newPix);
                        Pnm_ppmfree(&newPix);
                } else {
                        fprintf(stderr, "Rotation must be 0, 90, or 180\n");
                        CPUTime_Free(&timer);
                        Pnm_ppmfree(&pixFile);
                        return EXIT_FAILURE;
                }
        }
        if (time_file_name != NULL) {
                FILE *timingPointer = fopen(time_file_name, "a");
                average = time_used / (pixFile->width * pixFile->height);
                fprintf(timingPointer, "Total Time: %.0f \n", time_used);
                fprintf(timingPointer, "Average Pixel Time: %.0f \n \n", 
                                average);
                fclose(timingPointer);
        }
        CPUTime_Free(&timer);
        Pnm_ppmfree(&pixFile);
        return EXIT_SUCCESS;
}

/*
 * A2Methods_applyfun function to transform image 90 degrees clockwise
 */
void ninety(int i, int j, A2Methods_UArray2 oArray, void *elem, void *npFile)
{
        (void) elem;
                
        int height = (*(Pnm_ppm*)npFile)->methods->height(oArray);
        
        struct Pnm_rgb element = *(struct Pnm_rgb*)
                (*(Pnm_ppm*)npFile)->methods->at(oArray,i,j);
        
        int newCol = height - j - 1;
        int newRow = i; 

        *((struct Pnm_rgb *)(*(Pnm_ppm*)npFile)->methods->at(
                (*(Pnm_ppm*)npFile)->pixels, newCol, newRow)) = element;
}

/*
 * A2Methods_applyfun function to transform image 180 degrees clockwise 
 */
void one_eighty(int i, int j, A2Methods_UArray2 array, void *elem, void *pFile)
{
        (void) elem;
        int width = 
                (*(Pnm_ppm*)pFile)->methods->width((*(Pnm_ppm*)pFile)->pixels);
        int height = 
                (*(Pnm_ppm*)pFile)->methods->height((*(Pnm_ppm*)pFile)->pixels);
        struct Pnm_rgb element = *(struct Pnm_rgb*)
                (*(Pnm_ppm*)pFile)->methods->at((*(Pnm_ppm*)pFile)->pixels,i,j);
        
        int newCol = width - i - 1;
        int newRow = height - j - 1;
        
        *((struct Pnm_rgb *)(*(Pnm_ppm*)pFile)->methods->at(array,newCol,
                                newRow)) = element;
}
