/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors:
// NMec: 114597 &  88860  Name: Carlos Verenzuela & Rafael Claro
// Date: 24 de Novembro de 2023
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"
#include <math.h>
#include <stdint.h>

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
// 
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() { ///
  return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success = 
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
// 
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
// 
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
// 
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

//INICIO

Image ImageCreate(int width, int height, uint8 maxval) {                            ///aula 16 de N
  assert(width >= 0);
  assert(height >= 0);
  assert(0 < maxval && maxval <= PixMax);

  // Aloca memória para a estrutura da imagem
  Image img = (Image)malloc(sizeof(struct image));  

  if (img == NULL) {
    errCause = ("Failed to allocate memory in ImageCreate");
    free(img);
    return NULL;
  }

  // Inicializa os campos da estrutura da imagem
  img->width = width;
  img->height = height;
  img->maxval = maxval;

  // Aloca memória para o array de pixels
  img->pixel = (uint8*)malloc(sizeof(uint8) * width * height);
  if (img->pixel == NULL) {
    errCause = ("Failed to allocate memory for pixel array");
    free(img); // Libera a memória alocada para a estrutura da imagem
    return NULL;
  }

  // Inicializa os pixels para preto (0)
  for (int i = 0; i < width * height; i++) {
    img->pixel[i] = 0;
  }

  return img;
}


/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.

void ImageDestroy(Image* imgp) {                                   ///aula 16 de N
  assert(imgp != NULL);

  if (*imgp != NULL) { // Verifica se a imagem não é NULL
    free((*imgp)->pixel); // Libera a memória alocada para os pixels
    free(*imgp); // Libera a memória alocada para a estrutura da imagem
    *imgp = NULL; // Define o ponteiro como NULL para evitar acesso acidental
  }
  // Se (*imgp) for NULL, nenhum passo adicional é necessário
}

/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert (img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert (img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) {                  //dia 17
  assert(img != NULL);
  assert(min != NULL);
  assert(max != NULL);

  int width = img->width;
  int height = img->height;

  //Caso imagem vazia
  if ((width * height) == 0) {
    *min = 0; 
    *max = 0;
    return;
  }

  // Inicializa min e max com os valores extremos possíveis
  *min = PixMax;
  *max = 0;

  // Itera sobre todos os pixels da imagem para encontrar min e max
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // Atualiza min e max conforme necessário
      *min = (pixelValue < *min) ? pixelValue : *min;
      *max = (pixelValue > *max) ? pixelValue : *max;
    }
  }
}


/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) {                   // dia 17
  assert(img != NULL);

  // Verifica se as coordenadas são não negativas
  if (x < 0 || y < 0 || w <= 0 || h <= 0) {
    return 0;  // Área inválida
  }

  // Verifica se as coordenadas estão dentro dos limites da imagem
  if (x + w > img->width || y + h > img->height) {
    return 0;  // Área ultrapassa os limites da imagem
  }

  // Se ambas as condições anteriores forem atendidas, a área é válida
  return 1;
}


/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to 
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel. 
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {                                      // dia 17
  int index;

  // Verifica se as coordenadas estão dentro dos limites da imagem
  assert (0 <= x && x < img->width && 0 <= y && y < img->height);

  // Calcula o índice linear
  index = y * img->width + x;

  // Verifica se o índice está dentro dos limites da imagem
  assert (0 <= index && index < img->width * img->height);

  return index;
}


/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) {                         ///aula 16 de N
  assert(img != NULL);

  int size = img->width * img->height;
//Percorrer os píxeis e calcular o negativo de cada pixel
  for (int i = 0; i < size; ++i) {
    img->pixel[i] = img->maxval - img->pixel[i];
  }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) {        /// aula de 16 de nov
    assert(img != NULL);

    int width = img->width;
    int height = img->height;
    uint8 maxval = img->maxval;
//Percorre cada pixel e se p valor do pixel for menor,
 //define como preto, senão define como branco
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8 pixelValue = ImageGetPixel(img, x, y);

            // Aplicar a lógica de limiar
            if (pixelValue < thr) {
                ImageSetPixel(img, x, y, 0);  // Define como preto
            } else {
                ImageSetPixel(img, x, y, maxval);  // Define como branco
            }
        }
    }
}


/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) {            ///aula de 16 de nov
  assert(img != NULL);
  if(factor<0.0){factor=0.0;}
  int width = ImageWidth(img);
  int height = ImageHeight(img);
  // Percorre todos os pixels da imagem
  //for (int i = 0; i < img->width * img->height; ++i) {
    // Calcula o novo valor do pixel multiplicando pelo fator
    //uint8_t newPixelValue = (uint8_t)(img->pixel[i] * factor);

    // Arredonda o valor para int
    //int newPixelValueInt = (int)round(newPixelValue); 
    // Atualize o valor do pixel na imagem, tendo em conta que
    //não podem ter um valor maior a PixMax (255)
    //img->pixel[i] = (newPixelValueInt > PixMax) ? PixMax : newPixelValueInt;


  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // Obtenha o valor do pixel da imagem original
      uint8 pixelValue = ImageGetPixel(img, x, y);
      
      // Calcula o novo valor do pixel multiplicando pelo fator
      
      uint8_t newpixelValue = (uint8_t)(pixelValue * factor);
      if(newpixelValue>255){newpixelValue=255;}

      // Defina o pixel na imagem rotacionada
      ImageSetPixel(img, x, y, newpixelValue);
    }
  }
  //
}


/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
/// 
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint: 
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

Image ImageRotate(Image img) {                       ///aula de 16 de nov
  assert(img != NULL);

  // Obter as dimensões da imagem original
  int width = ImageWidth(img);
  int height = ImageHeight(img);

  //Nova imagem para armazenar a imagem rotacionada (width e height trocadas)
  Image rotatedImage = ImageCreate(height, width, ImageMaxval(img));

  // Verifica se a criação da nova imagem foi bem-sucedida
  if (rotatedImage == NULL) {
    errCause = "Memory allocation error (ImageRotate)";
    return NULL;
  }

  // Faça a rotação anti-horária de 90 graus
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // Obtenha o valor do pixel da imagem original
      uint8 pixelValue = ImageGetPixel(img, x, y);

      // Calcule as coordenadas na imagem rotacionada
      int rotatedX = height - y - 1;
      int rotatedY = x;

      // Defina o pixel na imagem rotacionada
      ImageSetPixel(rotatedImage, rotatedX, rotatedY, pixelValue);
    }
  }

  // Retorna a imagem rotacionada
  return rotatedImage;
}


/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

Image ImageMirror(Image img) {                                  ///aula de dia 16 de nov
  assert(img != NULL);

  // Cria uma nova imagem com as mesmas dimensões e valores máximos
  Image mirroredImg = ImageCreate(img->width, img->height, img->maxval);

  // Se falhar na criação da nova imagem, retorna NULL
  if (mirroredImg == NULL) {
    errCause = "Memory allocation error (ImageMirror)";
    return NULL;
  }

  //Percorre img
  for (int y = 0; y < img->height; y++) {
    for (int x = 0; x < img->width; x++) {
      // Calcula a posição espelhada
      int mirroredX = img->width - 1 - x;
      
      // Obtém o valor do pixel na posição original e
      // Define o valor do pixel na posição espelhada
      ImageSetPixel(mirroredImg, mirroredX, y, ImageGetPixel(img, x, y));
    }
  }
  return mirroredImg;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

                                                                    ///aula de 16 de nov

Image ImageCrop(Image img, int x, int y, int w, int h) {
  assert(img != NULL);
  assert(ImageValidRect(img, x, y, w, h));

  // Cria uma nova imagem para armazenar a subimagem cortada
  Image croppedImg = ImageCreate(w, h, img->maxval);
  if (croppedImg == NULL) {
    errCause = "Memory allocation error (ImageCrop)";
    return NULL;
  }

  // Copia os pixels da subimagem para a nova imagem
  for (int dy = 0; dy < h; dy++) {
    for (int dx = 0; dx < w; dx++) {
      int srcX = x + dx;
      int srcY = y + dy;
      int destIndex = dy * w + dx;
      int srcIndex = srcY * img->width + srcX;

      // Garante que os índices são válidos
      assert(srcX >= 0 && srcX < img->width);
      assert(srcY >= 0 && srcY < img->height);

      // Copia o pixel da subimagem para a nova imagem
      croppedImg->pixel[destIndex] = img->pixel[srcIndex];
    }
  }

  return croppedImg;
}


/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) {                                   ///aula de 16 ed nov
  assert(img1 != NULL); 
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));

  // Iterar sobre os pixels de img2 e colá-los em img1 na posição adequada
  for (int newY = 0; newY < img2->height; ++newY) {
    for (int newX = 0; newX < img2->width; ++newX) {
      // Calcular as coordenadas correspondentes em img1
      int destX = x + newX;
      int destY = y + newY;

      // Verificar se as coordenadas de destino são válidas em img1
      assert(ImageValidPos(img1, destX, destY));

      // Obter o valor do pixel em img2
      uint8 pixelValue = ImageGetPixel(img2, newX, newY);

      // Colar o pixel em img1 na posição correspondente
      ImageSetPixel(img1, destX, destY, pixelValue);
    }
  }
}


/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) {                  ///aula de 16 de nov
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
  //assert(alpha >= 0.0 && alpha <= 1.0);

  // Percorre os pixels de img2 e mistura no local apropriado em img1
  for (int i = 0; i < img2->height; ++i) {
    for (int j = 0; j < img2->width; ++j) {
      // Calcula a posição correspondente em img1
      int img1_x = x + j;
      int img1_y = y + i;

      // Verifica se a posição está dentro dos limites de img1
      if (ImageValidPos(img1, img1_x, img1_y)) {
        // Obtém os valores dos pixels de img1 e img2
        uint8 pixel1 = ImageGetPixel(img1, img1_x, img1_y);
        uint8 pixel2 = ImageGetPixel(img2, j, i);

        // Calcula o valor blend e atualiza o pixel em img1
        uint8 blended_pixel = (uint8)((1.0 - alpha) * pixel1 + alpha * pixel2);
        ImageSetPixel(img1, img1_x, img1_y, blended_pixel);
      }
    }
  }
}


/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) {
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidPos(img1, x, y));

  int subimgWidth = ImageWidth(img2);
  int subimgHeight = ImageHeight(img2);

  // Verifica se a subimagem cabe dentro da imagem maior
  if (!ImageValidRect(img1, x, y, subimgWidth, subimgHeight)) {
    errCause = "Subimagem não cabe (ImageMatchSubImage)";
    return 0;
  }

  // Percorre a região correspondente na imagem maior
  for (int i = 0; i < subimgWidth; i++) {
    for (int j = 0; j < subimgHeight; j++) {
      int img1PixelX = x + i;
      int img1PixelY = y + j;
      uint8 pixelImg1 = ImageGetPixel(img1, img1PixelX, img1PixelY);

      uint8 pixelImg2 = ImageGetPixel(img2, i, j);

      // Compara os pixels das duas imagens
      if (pixelImg1 != pixelImg2) {
        return 0; // Pixels diferentes, as imagens não coincidem
      }
    }
  }

  // Se chegou até aqui, as imagens coincidem
  return 1;
}


/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.

int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) {
  assert(img1 != NULL);
  assert(img2 != NULL);
  uint64_t count = 0;

  for (int y = 0; y <= img1->height - img2->height; ++y) {
    for (int x = 0; x <= img1->width - img2->width; ++x) {
      count += img2->width * img2->height;
      if (ImageMatchSubImage(img1, x, y, img2)) {
        // Subimage found, set the matching position and return 1
        *px = x;
        *py = y;
        printf("Total operations ImageLocateSubImage: %ld)\n", count);
        return 1;
      }
    }
  }

      // Se houver uma correspondência, define as posições e retorna 1
  uint64_t formula = (img1->width-img2->width+1)*(img1->height-img2->height+1)*(img2->width*img2->height);

  printf("Total operations ImageLocateSubImage: %ld \n", formula);

  return 0;  // No match found
}


/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.

void ImageBlur(Image img, int dx, int dy) { ///
  assert(img != NULL);
  assert(dx >= 0 && dy >= 0);
  int comps = 0;

  int width = img->width;
  int height = img->height;
  
  // Create a temporary image to store the blurred result
  Image blurredImage = ImageCreate(width, height, img->maxval);

  if (blurredImage == NULL) {
    // Memory allocation failed
    return; }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      double sum = 0;
      int count = 0;

      // Iterate over the pixels in the specified rectangle
      for (int j = -dy; j <= dy; ++j) {
        for (int i = -dx; i <= dx; ++i) {
          comps++;
          int nx = x + i;
          int ny = y + j;

          if (ImageValidPos(img, nx, ny)) {
            sum += ImageGetPixel(img, nx, ny);
            count++;
          }
        }
      }

      // Calcular a media e mandar o pixel para blurred image
      uint8 mean = (count > 0) ? (sum / count) : 0;
      ImageSetPixel(blurredImage, x, y, mean);
    }
  }

  // Copy the blurred image back to the original image
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      ImageSetPixel(img, x, y, ImageGetPixel(blurredImage, x, y));
    }
  }

  printf("Total operations Blur: %d (memory operations: %d)\n", width * height * (2 * dy + 1) * (2 * dx + 1), comps);

  // Destroy the temporary blurred image
  ImageDestroy(&blurredImage);
}
