#include <iostream>
#define cimg_display 0
#include "CImg.h"
#include "string"
#include <cmath>

#define PI 3.14159265

int rotateX (int x, int y, int r_x, float angle){
    return r_x + x*cos(-angle * PI / 180.0) - y*sin(-angle * PI / 180.0); 
}

int rotateY (int x, int y, int r_y, float angle){
    return r_y - x*sin(-angle * PI / 180.0) - y*cos(-angle * PI / 180.0); 
}

int main (int argc, char** argv){

    if(argc != 15) {
        throw std::runtime_error("Insufficient arguments");
    }

    const unsigned char red[] = {255, 0, 0};
    const unsigned char green[] = {0, 255, 0};
    const unsigned char blue[] = {0, 0, 255};
    const unsigned char pink[] = {255, 192, 203};
    const unsigned char yellow[] = {255, 255, 0};
    const unsigned char grey[] = {128, 128, 128};
    const unsigned char black[] = {0, 0, 0};

    cimg_library::CImg<unsigned char> img("map.jpg");

    //draw origin
    img.draw_circle(40, 1040, 15, black);

    // get rover location
    int x = 40 + atoi(argv[11]);
    int y = 1040 - atoi(argv[12]);
    // img.draw_rectangle(x-20, y-40, x+20, y+40, grey);

    //get the rotation
    float r = atof(argv[13]);

    //set rover width and height (half)
    int w = 30, h = 60;

    //draw rover with orientation
    cimg_library::CImg<int> points(4, 2);
    int thePoints[] = {rotateX(w,h,x,r),rotateY(w,h,y,r),rotateX(w,-h,x,r),rotateY(w,-h,y,r),rotateX(-w,-h,x,r),rotateY(-w,-h,y,r),rotateX(-w,h,x,r),rotateY(-w,h,y,r)};
    int *iterator = thePoints;
    cimg_forX(points, i) {
        points(i, 0) = *(iterator++);
        points(i, 1) = *(iterator++);
    }
    img.draw_polygon(points, grey);

    //draw red circle
    if(atoi(argv[1]) >= 0) img.draw_circle(40 + atoi(argv[1]), 1040 - atoi(argv[2]), 50, red);

    //draw green circle
    if(atoi(argv[3]) >= 0) img.draw_circle(40 + atoi(argv[3]), 1040 - atoi(argv[4]), 50, green);

    //draw blue circle
    if(atoi(argv[5]) >= 0) img.draw_circle(40 + atoi(argv[5]), 1040 - atoi(argv[6]), 50, blue);

    //draw pink circle
    if(atoi(argv[7]) >= 0) img.draw_circle(40 + atoi(argv[7]), 1040 - atoi(argv[8]), 50, pink);

    //draw yellow circle
    if(atoi(argv[9]) >= 0) img.draw_circle(40 + atoi(argv[9]), 1040 - atoi(argv[10]), 50, yellow);

    //draw scale line
    img.draw_rectangle(940,1010,1040,1016,black);

    //draw scale text
    img.draw_text(955, 1023, "%d mm", black, 0, 1, 23, atoi(argv[14]));
    
    img.save_jpeg("public/images/map.jpg");

}