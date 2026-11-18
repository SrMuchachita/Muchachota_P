// Un solo .c compila la implementacion real de stb_image_write (biblioteca
// de un solo header, dominio publico) — el resto del proyecto solo incluye
// el header. Se usa para guardar las capturas de pantalla en PNG en vez de
// BMP (mucho mas liviano, sin perder calidad).
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
