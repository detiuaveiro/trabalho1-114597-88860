#include <stdio.h>
#include <time.h>
#include "image8bit.h"
#include "image8bit.c"
// Função para medir o tempo
double measureTime(Image largeImage, Image smallImage) {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    ImageLocateSubImage(largeImage, NULL, NULL, smallImage);
    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    return cpu_time_used;
}

int main() {
    // Tamanho da imagem menor
    int smallImageWidth = 50;
    int smallImageHeight = 50;

    // Imagens para os tests
    Image smallImage = ImageCreate(smallImageWidth, smallImageHeight, 255);

    for (int i = 1; i <= 5; i++) {
        // Tamanho da imagem maior
        int largeImageWidth = smallImageWidth * i * 5;
        int largeImageHeight = smallImageHeight * i * 5;
        Image largeImage = createTestImage(largeImageWidth, largeImageHeight);

        // Medir o tempo e mostrar os resultados
        double timeUsed = measureTime(largeImage, smallImage);
        printf("Image size: %dx%d, Time: %f seconds\n", largeImageWidth, largeImageHeight, timeUsed);

        // Liberar memoria (imagem maior)
        ImageDestroy(&largeImage);
    }

    // Liberar memoria (imagem menor)
    ImageDestroy(&smallImage);

    return 0;
}