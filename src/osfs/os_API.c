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

// Escribe todo el buffer (2048 bytes) en el bloque num.
void write_block(void* buffer, unsigned int num);

// Lee todo un block (2048 bytes) y lo guarda en buffer
void read_block(void* buffer, unsigned int num);

// Imprimer los bits de byte
void print_bits(unsigned char byte);

// Imprime todo el bloque num (hex para hexadecimal)
void print_block(unsigned int num, short hex);

// Imprime todo el bitmap e imprime la cantidad de bloque libres.
void print_bitmap(short hex);

// Retorna la cantidad de espacio (bytes) libre en el disco (segun bitmap) 
unsigned int free_space();

// Retorna 0 si name no esta en el directorio de bloque dir,
// o el bloque en que está name (indice o directorio).
unsigned int is_child(unsigned int dir_block, char* name);

// Retorna 0 si name (en el dir_block) es un archivo, 1 si es un directorio, -1 si no existe
int is_dir_child(unsigned int dir_block, char* name);

// Retorna 0 si path es un archivo, 1 si es un directorio, -1 si no existe
int is_dir(char* path);

// Guarda en path_array el array de strings con los nombres de cada nivel del path.
void parse_path(char* path, char** path_array);

// Retorna el largo (depth) del path.
short path_length(char* path);

// Retorna 1 si path es valido (comienza con / y termina sin /, o es /)
int valid_path(char* path);

// Libera la memoria de cada string del array.
void free_array(char** array, int path_len);

// Asumiendo que dir es un bloque directorio imprime sus entradas.
void list_dir(unsigned int block);

// Asumiendo que path existe retorna su bloque (indice o directorio);
unsigned int get_block(char* path);

// Elimina la entrada del bloque de directorio donde esta path
void remove_entrance(char* path);

// Se asume que path es un archivo. Elimina recursivamente todo lo que tiene que ver 
// con el archivo. Actualiza bitmap. 
void remove_file(char* path);




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
    else print_bitmap(hex);
}

int os_exists(char* path){
    if (strcmp("/", path) && valid_path(path))
    {
        short path_len = path_length(path);
        char* path_array[path_len];
        parse_path(path, path_array);
        unsigned int dir = 0, dad;
        for (short i = 0; i < path_len; i++)
        {
            dad = dir;
            dir = is_child(dir, path_array[i]);
            if (!dir || (i < path_len - 1 && !is_dir_child(dad, path_array[i]))) {
                free_array(path_array, path_len);
                return 0;
            }
        }
        free_array(path_array, path_len);
    }
    return 1;
}

void os_ls(char* path){
    switch (is_dir(path))
    {
        case 1:;
        unsigned int block = get_block(path);
        list_dir(block);
        break;

        case 0:
        printf("%s es un archivo\n", path);
        break;

        case -1:
        printf("%s no existe\n", path);
        break;
    }
}

osFile* os_open(char* path, char mode){
    if (mode != 'r' && mode != 'w') {printf("Modo debe ser r o w\n"); return NULL;}
    int isdir = is_dir(path);
    if (isdir == -1 && mode != 'w') {printf("Archivo no existe\n"); return NULL;}
    if (isdir == 0 && mode == 'w') {printf("Archivo ya existe\n"); return NULL;}
    if (isdir == 1) {printf("Path no es un archivo\n"); return NULL;}
    
    osFile* file = osFile_init(path, mode);
    return file;
}

// int os_read(osFile* file, void* buffer, int nbytes){
//     if (file->size - file->virtual_cursor < nbytes){
//         return os_read(file, buffer, file->size - file->virtual_cursor);
//     }
//     unsigned int bytes_read = 0;
//     unsigned int buffer_cursor = 0;
//     while (nbytes - bytes_read >= 2048)
//     {
//         read_bytes(file, buffer, buffer_cursor, 2048);
//         bytes_read += 2048;
//         buffer_cursor += 2048;
//     }
//     if (nbytes - bytes_read){
//         read_bytes(file, buffer, buffer_cursor, nbytes - bytes_read);
//         bytes_read += nbytes - bytes_read;
//         buffer_cursor += nbytes - bytes_read;
//     }
//     return bytes_read;
// }

// int os_write(osFile* file, void* buffer, int nbytes){
//     if (nbytes > free_space() + 2048 - file->relative_cursor) {
//         return os_write(file, buffer, free_space() + 2048 - file->relative_cursor);
//     }
//     unsigned int bytes_written = 0;
//     unsigned int buffer_cursor = 0;
//     while (nbytes - bytes_written >= 2048)
//     {
//         write_bytes(file, buffer, buffer_cursor, 2048);
//         bytes_written += 2048;
//         buffer_cursor += 2048;
//     }
//     if (nbytes - bytes_written){
//         write_bytes(file, buffer, buffer_cursor, nbytes - bytes_written);
//         bytes_written += nbytes - bytes_written;
//         buffer_cursor += nbytes - bytes_written;
//     }
//     return bytes_written;
// }

// void os_close(osFile* file){
//     file_destroy(file);
// }

void os_rm(char* path){
    // valid path
    // es archivo
    unsigned int index_block = get_block(path);
    unsigned char buffer[2048];
    read_block(buffer, index_block);
    if (buffer[0] == 1) {
        remove_entrance(path);
        remove_file(path);
    }
    else {
        remove_entrance(path);
        buffer[0] --;
        write_block(buffer, index_block);
    }
}

int os_hardlink(char* orig, char* dest){
    // orig y dest son path validos
    // dest no existe
    // orig es un archivo (o quizas tambien un dir?)
    // Crear entrada para dest (si es que hay espacio en su dir)
    // get block de orig y agregarlo a la entrada de dest
    // aumentar las referencias en el bloque indice de orig
}

int os_mkdir(char* path){
    // path valido
    // existe el path hasta un nivel antes y es dentro de un dir
    // Crear entrada para el dir si hay espacio
    // Y asignarle un bloque libre (00000) para sus entradas
}

int os_rmdir(char* path, short recursive){
    // path valido 
    // path es un dir
    // si esta vacio eliminarlo
    // si no y recursive, eliminar todo recursivamente y luego eliminarlo
}

// int os_unload(char* orig, char* dest){}

// int os_load(char* orig){}



// Definicion de funciones internas de la API


void read_block(void* buffer, unsigned int num){
    FILE *file = fopen(DISK_NAME, "rb");
    fseek(file, num * 2048, SEEK_SET);
    fread(buffer, 2048, 1, file);
    fclose(file);
}

void print_bits(unsigned char byte){
    unsigned char mask = 1;
    char bits[9];
    for (int i = 7; i >= 0; i--) {
        bits[i] = mask & byte ? '1' : '0' ;
        mask = mask << 1;
    }
    bits[8] = '\0';
    fprintf(stderr, "%s ", bits);
}

void print_block(unsigned int num, short hex){
    unsigned char buffer[2048];
    read_block(buffer, num);
    if (hex) {
        for (int i = 0; i < 2048; i++)
        {
            if (buffer[i] >= 16) fprintf(stderr, "%X ", buffer[i]);
            else fprintf(stderr, "0%X ", buffer[i]);
        }
        fprintf(stderr, "\n");
    }
    else
    {
        for (int i = 0; i < 2048; i++) print_bits(buffer[i]);
        fprintf(stderr, "\n");
    }
    
}

void print_bitmap(short hex){
    for (int i = 1; i < 65; i++) print_block(i, hex);
    unsigned int freespace = free_space();
    fprintf(stderr, "Espacio libre en el disco: %u\n", freespace);
    fprintf(stderr, "Espacio utilizado en el disco: %u\n", (1 << 31) - freespace);
}

unsigned int free_space(){
    unsigned char buffer[2048];
    unsigned int free_blocks = 0;
    for (int i = 1; i < 65; i++)
    {
        read_block(buffer, i);
        for (int i = 0; i < 2048; i++){
            unsigned char mask = 1;
            for (int j = 0; j < 8; j++) {
                if (!(mask & buffer[i])) free_blocks ++;
                mask = mask << 1;
            }
        }
    }
    return free_blocks * (1 << 11);
}

void parse_path(char* path, char** path_array){
    char *token;
    token = strtok(path, "/");
    token = strtok(NULL, "/");
    int i = 0;
    while (token)
    {
        path_array[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(path_array[i], token);
        i++; 
        token = strtok(NULL, "/");
    }
}

short path_length(char* path){
    char *token;
    short len = 0;
    token = strtok(path, "/");
    token = strtok(NULL, "/");
    while (token)
    {
        len ++;
        token = strtok(NULL, "/");
    }
    return len;    
}

void free_array(char** array, int path_len){
    for (int i = 0; i < path_len; i++) free(array[i]);
}   