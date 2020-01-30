#include <stdio.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image.h"
#include "kernel.h"

void sobelOperator(Image& A, Image& B, Image& destination){
    ASSERT(A.width == B.width && A.width == destination.width);
    ASSERT(A.height == B.height && A.height == destination.height);

    for(int i = 0; i < A.width * A.height; ++i){
        r32 result = sqrt((A.data[i] * A.data[i]) + (B.data[i] * B.data[i]));
        CLAMP(result, 0, 255);

        destination.data[i] = result;
    }
}

void generateIntensityGradients(u8* buffer, Image& sobelX, Image& sobelY){
    for(int y = 0; y < sobelX.height; ++y){
        for(int x = 0; x < sobelX.width; ++x){
            r32 result = RAD2DEG(atan2f(sobelY.data[x + y * sobelX.width], sobelX.data[x + y * sobelX.width]));

            CLAMP(result, 0, 255);
            u8 byteResult = result;
            
            if(byteResult > 0 && byteResult <= 22){
                byteResult = 0;
            } else if(byteResult > 22 && byteResult <= 67){
                byteResult = 45;
            } else if(byteResult > 67 && byteResult <= 110){
                byteResult = 90;
            } else if(byteResult > 110 && byteResult <= 157){
                byteResult = 135;
            } else if(byteResult > 157 && byteResult <= 180){
                byteResult = 0;
            }

            buffer[x + y * sobelX.width] = byteResult;
        }
    }
}

u8 edgeThinning(Image& destination, u8* gradients, Image& gradientImage){
    // returns maximum gradient from the image so we can use it to determine the threshold
    u8 result = gradientImage.data[0];
    for(int y = 0; y < destination.height; ++y){
        for(int x = 0; x < destination.width; ++x){
            u8 localGradient = gradientImage.data[x + y * destination.width];

            i32 xx0 = x - 1;
            i32 xx1 = x + 1;

            i32 yy0 = y - 1;
            i32 yy1 = y + 1;

            CLAMP(xx0, 0, destination.width - 1);
            CLAMP(xx1, 0, destination.width - 1);

            CLAMP(yy0, 0, destination.height - 1);
            CLAMP(yy1, 0, destination.height - 1);

            if(localGradient >= 0 && localGradient <= 22){ 
                // vertical edge, check sides(left, right)
                // should round to 0 but, same thing
                if (gradientImage.data[xx0 + y * destination.width] > localGradient ||
                    gradientImage.data[xx1 + y * destination.width] > localGradient){
                    
                    gradientImage.data[x + y * destination.width] = 0;
                }
            } else if(localGradient > 22 && localGradient <= 67){
                // diagonal edge, check north east and south west 
                // should round to 45 but, same thing
                if (gradientImage.data[xx1 + yy0 * destination.width] > localGradient ||
                    gradientImage.data[xx0 + yy1 * destination.width] > localGradient){
                    
                    gradientImage.data[x + y * destination.width] = 0;
                }
            } else if(localGradient > 67 && localGradient <= 110){
                // horizontal, check north and south 
                // should round to 90 but, same thing
                if (gradientImage.data[x + yy0 * destination.width] > localGradient ||
                    gradientImage.data[x + yy1 * destination.width] > localGradient){
                    
                    gradientImage.data[x + y * destination.width] = 0;
                }
            } else if(localGradient > 110 && localGradient <= 135){
                // diagonal, check north west and south east
                // should round to 135 but, same thing
                if (gradientImage.data[xx0 + yy0 * destination.width] > localGradient ||
                    gradientImage.data[xx1 + yy1 * destination.width] > localGradient){
                    
                    gradientImage.data[x + y * destination.width] = 0;
                }
            }

            if(gradientImage.data[x + y * destination.width] > result){
                result = gradientImage.data[x + y * destination.width];
            }
        }
    }

    return result;
}

void tresholdPass(u8* gradientImage, u32 area, u8 lowerLimit, u8 upperLimit){
    for(int i = 0; i < area; ++i){
        if(gradientImage[i] <= lowerLimit){
            gradientImage[i] = 0;
        } else if(gradientImage[i] >= upperLimit){
            gradientImage[i] = 255;
        } else if(gradientImage[i] > lowerLimit && gradientImage[i] < upperLimit){
            gradientImage[i] = 128;
        }
    }
}

void canny(Image& source, Image& destination){
    // TODO(Sarmis)
    // can be done in 1 pass, a custom method with everything together, all kernels and all that
    Image sobelFinal = source;
    Image sobelX = source;
    Image sobelY = source;

    sobelFinal.data = new u8[destination.width * destination.height];
    sobelX.data = new u8[destination.width * destination.height];
    sobelY.data = new u8[destination.width * destination.height];

    u8* gradients = new u8[destination.width * destination.height];

    kernelPass(source.data, destination.data, destination.width, destination.height, KERNEL_GAUSSIAN_7x7, 7);
    
    kernelPass(destination.data, sobelX.data, sobelX.width, sobelX.height, KERNEL_SOBEL_X_3x3, 3);
    stbi_write_bmp("outputX.bmp", destination.width, destination.height, 1, sobelX.data);

    kernelPass(destination.data, sobelY.data, sobelY.width, sobelY.height, KERNEL_SOBEL_Y_3x3, 3);
    stbi_write_bmp("outputY.bmp", destination.width, destination.height, 1, sobelY.data);

    sobelOperator(sobelX, sobelY, sobelFinal);
    stbi_write_bmp("outputSobelFinal.bmp", destination.width, destination.height, 1, sobelFinal.data);

    generateIntensityGradients(gradients, sobelX, sobelY);

    u8 upperLimit = ((r32)(edgeThinning(destination, gradients, sobelFinal)) * 0.7);
    u8 lowerLimit = ((r32)(upperLimit) * 0.5);

    stbi_write_bmp("output_before.bmp", destination.width, destination.height, 1, sobelFinal.data);

    printf("limits [%d %d]\n", upperLimit, lowerLimit);
    fflush(stdout);

    tresholdPass(sobelFinal.data, destination.width * destination.height, lowerLimit, upperLimit);

    destination.data = sobelFinal.data;

    // delete[] gradients;
    delete[] sobelX.data;
    delete[] sobelY.data;
}

void clone(Image& source, Image& destination){
    destination.width = source.width;
    destination.height = source.height;
    
    for(int i = 0; i < destination.width * destination.height; ++i){
        destination.data[i] = source.data[i];
    }
}

int main(){
    Image source;

    source.data = stbi_load("image3.jpg", &source.width, &source.height, NULL, 1);

    Image destination = source;
    destination.data = new u8[destination.width * destination.height];


    canny(source, destination);


    stbi_write_bmp("output.bmp", destination.width, destination.height, 1, destination.data);

    stbi_image_free(source.data);

    delete[] destination.data;

    return 0;
}










