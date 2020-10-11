#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_API.h"


// Variable global con el nombre del disco
char* DISK_NAME;


// Declaración de funciones de osFile

// Crea una nueva instancia de osFile.
osFile* osFile_init(char* path, char mode);

// Lee nbytes de file y los guarda en buffer desde buffer cursor
void read_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes);

// Escribe nbytes de buffer desde buffer cursor en file.
// Busca bloques libres para seguir escribiendo y los marca como usados en el bitmap.
// Actualiza el size de file.
void write_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes);


// Declaración de funciones internas de la API

// Imprime todo el bloque num (hex para hexadecimal)
void print_block(unsigned int num, short hex);

// Imprime todo el bitmap e imprime la cantidad de bloque libres.
void print_bitmap();

// Retorna 0 si name no esta en el directorio de bloque dir,
// o el bloque en que está path (indice o directorio).
unsigned int exists(unsigned int dir_block, char* name);

// Retorna 0 si name (en el dir_block) es un archivo, 1 si es un directorio, -1 si no existe
int is_dir_child(unsigned int dir_block, char* name);

// Retorna 0 si path es un archivo, 1 si es un directorio, -1 si no existe
int is_dir(char* path);

// Guarda en path_array el array de strings con los nombres de cada nivel del path.
void parse_path(char* path, char** path_array);

// Retorna el largo (depth) del path.
short path_length(char* path);

// Libera la memoria del array de strings y de cada string del array.
void free_array(char** array);

// Asumiendo que dir es un bloque directorio imprime sus entradas.
void list_dir(unsigned int block);

// Asumiendo que path existe retorna su bloque (indice o directorio);
unsigned int get_block(char* path);

// Retorna la cantidad de espacio (bytes) libre en el disco (segun bitmap) 
int free_space();



// Definicion de Funciones expuestas de la API

void os_mount(char* diskname){
    DISK_NAME = malloc(strlen(diskname) + 1); 
    strcpy(DISK_NAME, diskname);
}
void os_unmount(){
    free(DISK_NAME);
}

void os_bitmap(unsigned short num, short hex){
    if (num) print_block(num, hex);
    // else print_bitmap();
}

// int os_exists(char* path){
//     if (!strcmp("/", path))
//     {
//         short path_len = path_length(path);
//         char** path_array = calloc(path_len, sizeof(char*));
//         parse_path(path, path_array);
//         unsigned int dir = 0, dad;
//         for (short i = 0; i < path_len; i++)
//         {
//             dad = dir;
//             dir = exists(dir, path_array[i]);
//             if (!dir || (i < path_len - 1 && !is_dir_child(dad, path_array[i]))) {
//                 free_array(path_array);
//                 return 0;
//             }
//         }
//         free_array(path_array);
//     }
//     return 1;
// }

// void os_ls(char* path){
//     switch (is_dir(path))
//     {
//         case 1:;
//         unsigned int block = get_block(path);
//         list_dir(block);
//         break;
            
//         case 0:
//         printf("%s es un archivo\n", path);
//         break;

//         case -1:
//         printf("%s no existe\n", path);
//         break;
//     }
// }

// osFile* os_open(char* path, char mode){
//     if (mode != 'r' && mode != 'w') {printf("Modo debe ser r o w\n"); return NULL;}
//     int isdir = is_dir(path);
//     if (isdir == -1) {printf("Archivo no existe\n"); return NULL;}
//     if (isdir == 1) {printf("Path no es un archivo\n"); return NULL;}
    
//     osFile* file = osFile_init(path, mode);
//     return file;
// }

// int os_read(osFile* file_desc, void* buffer, int nbytes){
//     if (file_desc->size - file_desc->virtual_cursor < nbytes){
//         return os_read(file_desc, buffer, file_desc->size - file_desc->virtual_cursor);
//     }
//     unsigned int bytes_read = 0;
//     unsigned int buffer_cursor = 0;
//     while (nbytes - bytes_read >= 2048)
//     {
//         read_bytes(file_desc, buffer, buffer_cursor, 2048);
//         bytes_read += 2048;
//         buffer_cursor += 2048;
//     }
//     if (nbytes - bytes_read){
//         read_bytes(file_desc, buffer, buffer_cursor, nbytes - bytes_read);
//         bytes_read += nbytes - bytes_read;
//         buffer_cursor += nbytes - bytes_read;
//     }
//     return bytes_read;
// }

// int os_write(osFile* file_desc, void* buffer, int nbytes){
//     if (nbytes > free_space() + 2048 - file_desc->relative_cursor) {
//         return os_write(file_desc, buffer, free_space() + 2048 - file_desc->relative_cursor);
//     }
//     unsigned int bytes_written = 0;
//     unsigned int buffer_cursor = 0;
//     while (nbytes - bytes_written >= 2048)
//     {
//         write_bytes(file_desc, buffer, buffer_cursor, 2048);
//         bytes_written += 2048;
//         buffer_cursor += 2048;
//     }
//     if (nbytes - bytes_written){
//         write_bytes(file_desc, buffer, buffer_cursor, nbytes - bytes_written);
//         bytes_written += nbytes - bytes_written;
//         buffer_cursor += nbytes - bytes_written;
//     }
//     return bytes_written;
// }

// int os_close(osFile* file_desc){}

// int os_rm(char* path){}

// int os_hardlink(char* orig, char* dest){}

// int os_mkdir(char* path){}

// int os_rmdir(char* path, short recursive){}

// int os_unload(char* orig, char* dest){}

// int os_load(char* orig){}



// Definicion de funciones internas de la API

void print_block(unsigned int num, short hex){
    FILE *file = fopen(DISK_NAME, "rb");
    fseek(file, num * 2048, SEEK_SET);
    unsigned char buffer[2048];
    fread(buffer, 2048, 1, file);
    fclose(file);
    for (int i = 0; i < 2048; i++)
    {
        if (buffer[i] >= 16) printf("%X ", buffer[i]);
        else printf("0%X ", buffer[i]);
    }
    printf("\n");
}