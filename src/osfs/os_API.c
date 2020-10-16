#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_API.h"

enum tipo {ARCHIVO, DIR};

// Variable global con el nombre del disco
char* DISK_NAME;


// Declaración de funciones de osFile

// Crea una nueva instancia de osFile. 
osFile* osFile_init(char* path, char mode);

// Lee nbytes de file (maximo 2048), asumiendo que se puede y 
// los guarda en buffer desde buffer cursor. Actualiza toda la info de file
void read_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes);

// Escribe nbytes (2048 max) de buffer desde buffer cursor en file.
// Busca bloques libres para seguir escribiendo y los marca como usados en el bitmap.
// Actualiza el size de file.
void write_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes);

// Incremente el realtive cursor (en 2048) al siguiente data block (en 0).
void INC_cursor(osFile* file);

// Se ejecuta al hacer el primer os_write con un archivo en modo w.
// destina bloques libres para el primer direc block y el primer 
// data block de ese direc block.
void first_write(osFile* file);

// Libera la memoria de file
void file_destroy(osFile* file);


// Declaración de funciones internas de la API

// Escribe todo el buffer (2048 bytes) en el bloque num.
void write_block(void* buffer, unsigned int num);

// Lee todo un block (2048 bytes) y lo guarda en buffer
void read_block(void* buffer, unsigned int num);

// Escribe todo el buffer (512 unsigned ints) en el bloque num en big endian.
void write_pointers_block(unsigned int* buffer, unsigned int num);

// Lee todo un block (como 512 unsigned ints en big endian) y lo guarda en buffer
void read_pointers_block(unsigned int* buffer, unsigned int num);

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

// Crea una entrada para name en parent, con el tipo y su bloque(22bits).
void create_entrance(char* parent, char* name, int tipo, unsigned int block);

// Elimina la entrada del bloque de directorio donde esta path
void remove_entrance(char* path);

// Crea un archivo nuevo (su entrada y su bloque indice con size 0 y bitmap).
void create_file(char* path);

// Se asume que path es un archivo. Elimina recursivamente todo lo que tiene que ver 
// con el archivo. Actualiza bitmap. 
void remove_file(char* path);

// Guarda en parent el string con el path hasta el penultimo nivel
void get_parent(char* path, char* parent);

// Retorna un numero de bloque libre y lo marca como usado en el bitmap.
unsigned int get_free_block();

// Pasa num a big endian y lo guarda en buffer de bytes bytes.
void to_big_endian(unsigned char* buffer, unsigned long long num, int bytes);

// Pasa buffer en big endian a little endian y retorna el numero.
unsigned long long to_little_endian(unsigned char* buffer, int bytes);

// Acutaliza el bitmap del block con el valor value (1 o 0)
void update_bitmap(unsigned int block, int value);




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

// int os_exists(char* path){
//     if (strcmp("/", path) && valid_path(path))
//     {
//         short path_len = path_length(path);
//         char* path_array[path_len];
//         parse_path(path, path_array);
//         unsigned int dir = 0, dad;
//         for (short i = 0; i < path_len; i++)
//         {
//             dad = dir;
//             dir = is_child(dir, path_array[i]);
//             if (!dir || (i < path_len - 1 && !is_dir_child(dad, path_array[i]))) {
//                 free_array(path_array, path_len);
//                 return 0;
//             }
//         }
//         free_array(path_array, path_len);
//     }
//     return 1;
// }

void os_ls(char* path){
    unsigned int block = get_block(path);
    printf("Listando: %s\n", path);
    list_dir(block);
}

osFile* os_open(char* path, char mode){
    // Revisar si es valido hasta el penultimo nivel
    // if (mode != 'r' && mode != 'w') {printf("Modo debe ser r o w\n"); return NULL;}
    // int isdir = is_dir(path);
    // if (isdir == -1 && mode == 'r') {printf("Archivo no existe\n"); return NULL;}
    // if (isdir == 0 && mode == 'w') {printf("Archivo ya existe\n"); return NULL;}
    // if (isdir == 1) {printf("Path no es un archivo\n"); return NULL;}
    
    if (mode == 'w') create_file(path);
    osFile* file = osFile_init(path, mode);
    return file;
}

int os_read(osFile* file, void* buffer, int nbytes){
    if (file->mode != 'r') return 0;
    if (nbytes == 0) return 0;
    if (file->size - file->virtual_cursor < nbytes){
        return os_read(file, buffer, file->size - file->virtual_cursor);
    }
    unsigned int bytes_read = 0;
    unsigned int buffer_cursor = 0;
    while (nbytes - bytes_read >= 2048)
    {
        read_bytes(file, buffer, buffer_cursor, 2048);
        bytes_read += 2048;
        buffer_cursor += 2048;
    }
    if (nbytes - bytes_read){
        read_bytes(file, buffer, buffer_cursor, nbytes - bytes_read);
        bytes_read += nbytes - bytes_read;
        buffer_cursor += nbytes - bytes_read;
    }
    return bytes_read;
}

int os_write(osFile* file, void* buffer, int nbytes){
    if (file->mode != 'w') return 0;
    if (nbytes == 0) return 0;
    if (nbytes > free_space() + 2048 - file->relative_cursor) {
        return os_write(file, buffer, free_space() + 2048 - file->relative_cursor);
    }
    unsigned int bytes_written = 0;
    unsigned int buffer_cursor = 0;
    while (nbytes - bytes_written >= 2048)
    {
        write_bytes(file, buffer, buffer_cursor, 2048);
        bytes_written += 2048;
        buffer_cursor += 2048;
    }
    if (nbytes - bytes_written){
        write_bytes(file, buffer, buffer_cursor, nbytes - bytes_written);
        bytes_written += nbytes - bytes_written;
        buffer_cursor += nbytes - bytes_written;
    }
    return bytes_written;
}

void os_close(osFile* file){
    if (file->mode == 'w'){
        write_pointers_block(file->data_blocks, file->direc_blocks[file->n_direc_block]);
        write_pointers_block(file->direc_blocks, file->index_block);
        unsigned char buffer[2048];
        read_block(buffer, file->base_index_block);
        buffer[0] = 1;
        to_big_endian(&buffer[1], file->size, 7);
        write_block(buffer, file->base_index_block);
    }
    file_destroy(file);
}

// void os_rm(char* path){
//     // valid path
//     // es archivo
//     unsigned int index_block = get_block(path);
//     unsigned char buffer[2048];
//     read_block(buffer, index_block);
//     if (buffer[0] == 1) {
//         remove_entrance(path);
//         remove_file(path);
//     }
//     else {
//         remove_entrance(path);
//         buffer[0] --;
//         write_block(buffer, index_block);
//     }
// }

// int os_hardlink(char* orig, char* dest){
//     // orig y dest son path validos
//     // dest no existe
//     // orig es un archivo (o quizas tambien un dir?)
//     // Crear entrada para dest (si es que hay espacio en su dir)
//     // get block de orig y agregarlo a la entrada de dest
//     // aumentar las referencias en el bloque indice de orig
// }

// int os_mkdir(char* path){
//     // path valido
//     // existe el path hasta un nivel antes y es dentro de un dir
//     // Crear entrada para el dir si hay espacio
//     // Y asignarle un bloque libre (00000) para sus entradas
// }

// int os_rmdir(char* path, short recursive){
//     // path valido 
//     // path es un dir
//     // si esta vacio eliminarlo
//     // si no y recursive, eliminar todo recursivamente y luego eliminarlo
// }

// int os_unload(char* orig, char* dest){}

// int os_load(char* orig){}


// Definicion de funciones de osFile

osFile* osFile_init(char* path, char mode){
    osFile* file = calloc(1, sizeof(osFile));
    file->base_index_block = get_block(path);
    file->index_block = file->base_index_block;
    file->mode = mode;
    file->virtual_cursor = 0;
    file->relative_cursor = 0;
    file->n_index_block = 0;
    file->n_direc_block = 2;
    file->n_data_block = 0;
    if (mode == 'r') {
        read_pointers_block(file->direc_blocks, file->base_index_block);
        read_pointers_block(file->data_blocks, file->direc_blocks[2]);
        file->size = (((unsigned long long) file->direc_blocks[0] << 32) | file->direc_blocks[1]) 
                    & (((unsigned long long) 1 << 56) - 1);
    }
    else {
        file->size = 0;
    }
    return file;
}

void read_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes){
    if (nbytes == 0) return;
    if (file->relative_cursor + nbytes <= 2048){
        unsigned char block_buffer[2048];
        read_block(block_buffer, file->data_blocks[file->n_data_block]);
        memcpy((unsigned char*) buffer + buffer_cursor, &block_buffer[file->relative_cursor], nbytes);
        file->relative_cursor += nbytes;
        file->virtual_cursor += nbytes;
    }
    else
    {
        unsigned int bytes_restantes = nbytes - 2048 + file->relative_cursor;
        read_bytes(file, buffer, buffer_cursor, 2048 - file->relative_cursor);
        INC_cursor(file);
        read_bytes(file, buffer, buffer_cursor + nbytes - bytes_restantes, bytes_restantes);
    }   
}

void write_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes){
    if (nbytes == 0) return;
    if (file->virtual_cursor == 0 && file->direc_blocks[2] == 0) first_write(file);
    if (file->relative_cursor + nbytes <= 2048){
        unsigned char block_buffer[2048];
        read_block(block_buffer, file->data_blocks[file->n_data_block]);
        memcpy(&block_buffer[file->relative_cursor], (unsigned char*) buffer + buffer_cursor, nbytes);
        write_block(block_buffer, file->data_blocks[file->n_data_block]);
        file->relative_cursor += nbytes;
        file->virtual_cursor += nbytes;
        file->size += nbytes;
    }
    else
    {
        unsigned int bytes_restantes = nbytes - 2048 + file->relative_cursor;
        write_bytes(file, buffer, buffer_cursor, 2048 - file->relative_cursor);
        INC_cursor(file);
        write_bytes(file, buffer, buffer_cursor + nbytes - bytes_restantes, bytes_restantes);
    }
}

void first_write(osFile* file){
    file->direc_blocks[2] = get_free_block();
    file->data_blocks[0] = get_free_block();
}

void INC_cursor(osFile* file){
    file->relative_cursor = 0;
    if (file->n_data_block < 511) {
        file->n_data_block ++;
        if (file->mode == 'w') file->data_blocks[file->n_data_block] = get_free_block();
    }
    else {
        if (file->mode == 'w') write_pointers_block(file->data_blocks, file->direc_blocks[file->n_direc_block]);
        file->n_data_block = 0;
        if (file->n_direc_block < 510) {
            file->n_direc_block ++;
            if (file->mode == 'w') {
                file->direc_blocks[file->n_direc_block] = get_free_block();
                file->data_blocks[0] = get_free_block();
            }
            else read_pointers_block(file->data_blocks, file->direc_blocks[file->n_direc_block]);            
        }
        else
        {
            file->n_direc_block = 0;
            if (file->mode == 'w'){
                file->direc_blocks[511] = get_free_block();
                write_pointers_block(file->direc_blocks, file->index_block);
                file->index_block = file->direc_blocks[511];
                file->direc_blocks[0] = get_free_block();
                file->data_blocks[0] = get_free_block();
            }
            else {
                file->index_block = file->direc_blocks[511];
                read_pointers_block(file->direc_blocks, file->direc_blocks[511]);
                read_pointers_block(file->data_blocks, file->direc_blocks[0]);
            }
            file->n_index_block ++;
        }
    }
}

void file_destroy(osFile* file){
    free(file);
}


// Definicion de funciones internas de la API

void read_block(void* buffer, unsigned int num){
    FILE *file = fopen(DISK_NAME, "rb");
    fseek(file, num * 2048, SEEK_SET);
    fread(buffer, 2048, 1, file);
    fclose(file);
}

void write_block(void* buffer, unsigned int num){
    FILE *file = fopen(DISK_NAME, "wb");
    fseek(file, num * 2048, SEEK_SET);
    fwrite(buffer, 2048, 1, file);
    fclose(file);
}

void read_pointers_block(unsigned int* buffer, unsigned int num){
    unsigned char bytes_buffer[2048];
    read_block(bytes_buffer, num);
    for (int i = 0; i < 512; i++)
    {
        unsigned char buf[4];
        memcpy(buf, &bytes_buffer[4 * i], 4);
        unsigned int number = to_little_endian(buf, 4);
        buffer[i] = number;
    }
}

void write_pointers_block(unsigned int* buffer, unsigned int num){
    unsigned char bytes_buffer[2048];
    for (int i = 0; i < 512; i++)
    {
        unsigned char buf[4];
        to_big_endian(buf, buffer[i], 4);
        memcpy(&bytes_buffer[4 * i], buf, 4);
    }
    write_block(bytes_buffer, num);
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
    char path_cp[strlen(path) + 1];
    strcpy(path_cp, path);
    token = strtok(path_cp, "/");
    int i = 0;
    while (token)
    {
        path_array[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(path_array[i], token);
        i++; 
        token = strtok(NULL, "/");
    }
}



void free_array(char** array, int path_len){
    for (int i = 0; i < path_len; i++) free(array[i]);
}

void create_file(char* path){
    unsigned int block = get_free_block();
    char parent[strlen(path) + 1];
    get_parent(path, parent);
    int path_len = path_length(path);
    char* path_array[path_len];
    parse_path(path, path_array);
    char* name = path_array[path_len];
    path_length(path);
    create_entrance(parent, name, ARCHIVO, block);
    free_array(path_array, path_len);
}

void create_entrance(char* parent, char* name, int tipo, unsigned int block){
    
}

void get_parent(char* path, char* parent){
    int path_len = path_length(path);
    if (path_len == 1) {strcpy(parent, "/"); return;}
    char* path_array[path_len];
    parse_path(path, path_array);
    int cursor = 0;
    for (int i = 0; i < path_len - 1; i++)
    {
        parent[cursor] = '/'; cursor++;
        strcpy(&parent[cursor], path_array[i]); cursor += strlen(path_array[i]);
    }
}

unsigned int get_free_block(){
    unsigned int free_block; 
    for (int block = 1; block < 65; block++)
    {
        unsigned char buffer[2048];
        read_block(buffer, block);
        for (int byte = 0; byte < 2048; byte++)
        {
            if (buffer[byte] != 255)
            {
                unsigned char mask = 1;
                for (int bit = 0; bit < 8; bit++)
                {
                    mask = mask << bit;
                    if (!(buffer[byte] & mask)){
                        free_block = (block << 14) | (byte << 3) | (7 - bit);
                        buffer[byte] = buffer[byte] | mask;
                        write_block(buffer, block);
                        return free_block;
                    }
                }
            }
        }
    }
    return 0;
}

// void update_bitmap(unsigned int block, int value){

// }

void to_big_endian(unsigned char* buffer, unsigned long long num, int bytes){
    for (int i = 0; i < bytes; i++) {
        buffer[i] = (unsigned char) (num & (255 << ((bytes - i - 1) * 8))) >> ((bytes - i - 1) * 8);
    }
}

unsigned long long to_little_endian(unsigned char* buffer, int bytes){
    unsigned long long number = 0;
    for (int i = 0; i < bytes; i++) {
        number += (unsigned long long) buffer[i] << ((bytes - i - 1) * 8);
    }
    return number;
}

unsigned int get_block_in_dir(char* name, unsigned int bloque_dir){
    unsigned char buffer[2048];
    read_block(buffer, bloque_dir);
    for (int i = 0; i < 64; i++)
    {
        unsigned char mask = 192;
        if (buffer[32 * i] & mask) {
            if (!strcmp((char*)&buffer[32 * i + 3], name)){
                unsigned int block = (((unsigned int) (buffer[32 * i] & 63)) << 16) | \
                                     ((unsigned int) buffer[32 * i + 1]) << 8 | \
                                     buffer[32 * i + 2];
                return block;
            }
        }
    }
    return 0;
}

unsigned int get_block(char* path){
    if (!strcmp(path, "/")) return 0;
    int path_len = path_length(path);
    char* path_array[path_len];
    parse_path(path, path_array);
    unsigned int dir = 0;
    for (int i = 0; i < path_len; i++) dir = get_block_in_dir(path_array[i], dir); 
    free_array(path_array, path_len);
    return dir;
}

short path_length(char* path){
    char *token;
    char path_cp[strlen(path) + 1];
    strcpy(path_cp, path);
    short len = 0;
    token = strtok(path_cp, "/");
    while (token)
    {
        len ++;
        token = strtok(NULL, "/");
    }
    return len;
}

void list_dir(unsigned int block){
    unsigned char buffer[2048];
    read_block(buffer, block);
    int first = 1;
    printf("tipo nombre\n");
    for (int i = 0; i < 64; i++)
    {
        unsigned char mask = 192;
        if (buffer[32 * i] & mask) {
            if (!first) printf("\n"); else first = 0;            
            printf("%d    ", (buffer[32 * i] & mask) >> 6);
            printf("%s", &buffer[32 * i + 3]);
        }
    }
}

// int os_mkdir(char* path){
//     // path valido
//     // existe el path hasta un nivel antes y es dentro de un dir
//     // Crear entrada para el dir si hay espacio
//     // Y asignarle un bloque libre (00000) para sus entradas
//     if (valid_path(path))
//     {   
//         int len_path = path_length(path);
//         char** path_array[len_path] , new_path_array[len_path - 1];
//         char* prev_path ;
//         char* name;

//         parse_path(path,path_array);
//         //    copio el path sin lo "nuevo"
//         for (int i = 0; i < len_path - 1; i++) new_path_array[i] = path_array[i];

//         //    tengo el path previo(me faltaaaa) y ahora quiero saber el puntero del bloque

//         unsigned int index_block = get_block(prev_path), new_block;
//         unsigned char buffer[2048];
//         read_block(buffer, index_block);

//         for (int i = 0; i < 64; i++)
//         {
//             unsigned char mask = 192;
//             if (!(buffer[32 * i] & mask)) {
//                 unsigned int block = get_free_block();
//                 unsigned char buf[3];
//                 to_big_endian(buf, block, 3);
//                 memcpy(&buffer[32 * i], buf, 3);
//                 buffer[32 * i] = ((unsigned char) 128) | buffer[32 * i];
//                 strcpy(&buffer[32 * i + 3], name);
//                 break;
//             }
//         }
//         write_block(buffer, index_block);

//         //     ahora tengo en el buffer toda la info del bloque, ahora tengo que encontrar una entrada que tenga 00 
//         //     en los 2 primeros bits
//         unsigned int count = 0 ;
//         //    el 256 es 2 bits + 22 bits + 29 bytes * 8, con esto llego al comienzo de la sigueinte entrada 
//         while (buffer[count] != 0 && buffer[count + 1] != 0) count += 256;
//         //     count almacena la primera entrada desocupada, asumo que siempre hay una!
//         //     tengo que actualizar ahora esa entrada  , el block pointer y el nombre
//         //     ponerle un directorio
//         buffer[count] ++;
//         //    ponerle el block pointer y cambiar el bitmap de 0 a 1 (la idea es que la función lo haga)

//         new_block = get_free_bitmap()

//         //    ponerle el nombre
//     }
//     else printf("path :%s no es valido\n", path);
// }
