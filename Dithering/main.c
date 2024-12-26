#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define error(new_value,old_value) ((old_value - new_value))

int FloydSteinberg(FILE* input_img, FILE* output_img, int width, int height);


int main() {

    int width;
    int height;

    FILE* input_img;
    FILE* output_img;

    // open files
    input_img = fopen("input.ppm", "r");
    output_img = fopen("output.ppm", "w");

    // store the file header type
    putc(getc(input_img),output_img);
    putc(getc(input_img),output_img);
    putc(getc(input_img),output_img);
   
    // store width and height
    fscanf(input_img,"%d",&width);
    fprintf(output_img,"%d\n",width);
    
    fscanf(input_img,"%d",&height);
    fprintf(output_img,"%d\n255\n",height);

    // get rid of the chars "\n255\n"
    getc(input_img);
    getc(input_img);
    getc(input_img);
    getc(input_img);
    getc(input_img);
    
    // print some infos for the user
    printf( "%d, %d \n",height,width);
    printf( "Runnin ... \n");

    // Apply Floyd-Steinberg dithering
    FloydSteinberg( input_img, output_img, width, height);


    // close files
    fclose(input_img);
    fclose(output_img);
    printf( "End \n");

    // Quit the program
    ExitToShell();
    return 0;

}

// Applies Floyd-Steinberg dithering to the image
int FloydSteinberg(FILE* input_img, FILE* output_img, int width, int height) {
    
    // variables used in the for loops, they must be  
    // for (int x = 0; x < width; x++) is not possible in this version of C
    int y;
    int x;
    

    // pointers to arrays
    float* errors;
    float* errors_next;
    float* tmp;

    uint8_t* image;

    // old value is with added error
    // new value is value which will be stored in the ne image
    float old_value; 
    uint8_t new_value; 

    // alocate error array and set it to zeros
    errors = calloc(width, sizeof(float));
    if (!errors) {
        fprintf(stderr, "Memory allocation errors failed\n");
        return 1;
    }

    // alocate second error array and set it to zeros
    errors_next = calloc(width, sizeof(float));
    if (!errors) {
        fprintf(stderr, "Memory allocation errors_bext failed\n");
        return 1;
    }

    // alocate array for line of an image
    image = malloc(width*3*sizeof(uint8_t));
    if (!image) {
        fprintf(stderr, "Memory allocation image_line failed\n");
        return 1;
    }

    

    for (y = 0; y < height; y++) {
        
        // read line of image (width times 3 pixels size))
        fread(image, width*3,1,input_img);
        for (x = 0; x < width; x++) {

            // compute average over the R, G, B values
            // ass the error
            old_value = (image[3*x] + image[3*x + 1] + image[3*x + 2]) / 3 + errors[x];

            // new value is 255, if old_value >= 128 and it is 0 if not 
            new_value = (255 * ( old_value >= 128));

            // store data for RGB values
            // (they are the same, since we want black or white)
            putc((uint8_t)new_value, output_img); // RED
            putc((uint8_t)new_value, output_img); // GREEN
            putc((uint8_t)new_value, output_img); // BLUE


            // Error difusion
            if (x + 1 < width){
                // difuse error in the current line
                errors[x+1] = errors[x+1] + error(new_value,old_value) * 7 / 16;
            } 

            if (y + 1 < height) {
                // difuse error in the line below

                //
                if (x > 0){
                    // update error of line below and one pixel to the left
                    errors_next[x-1] = errors_next[x-1] + error(new_value,old_value) * 3/16;
                }else{
                    // if x is zero, reset the value in of the error, so the error does not accumulate over lines
                    // the value will be set to new value in further part of the code
                    errors_next[x] = 0;
                }

                // update error of pixel below
                errors_next[x] = errors_next[x] + error(new_value,old_value) * 5/16;

                if (x + 1 < width){
                    // set error of line below and one pixel to the right
                    // mind, there is not update of the error as in previous cases, this is
                    // setting the value to zero and updating at one step
                    errors_next[x+1] = error(new_value,old_value) * 1/16;
                }
            }
        }

        // switch the pointers to error arrays
        tmp = errors_next;
        errors_next = errors;
        errors = tmp;
    }

    // free all three arrays (2 for errors nd one for the image line)
    free(image);
    free(errors);
    free(errors_next);
    return 0;
}
