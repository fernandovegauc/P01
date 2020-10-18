#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_API.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>

/* -------- Variables globales --------- */

// Variable global con el nombre del disco
char* DISK_NAME;


/* -------- Declaración de funciones de osFile --------- */

// Crea una nueva instancia de osFile. 
osFile* osFile_init(char* path, char mode);

// Lee nbytes de file (maximo 2048), asumiendo que se puede y 
// los guarda en buffer desde buffer cursor. Actualiza toda la info de file
void read_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes);

// Escribe nbytes (2048 max) de buffer desde buffer cursor en file.
// Busca bloques libres para seguir escribiendo y los marca como usados en el bitmap.
// Actualiza el size de file.
void write_bytes(osFile* file, void* buffer, unsigned int buffer_cursor, int nbytes);

// Incrementa el realtive cursor (en 2048) al siguiente data block (en 0).
void INC_cursor(osFile* file);

// Se ejecuta al hacer el primer os_write con un archivo en modo w.
// destina bloques libres para el primer direc block y el primer 
// data block de ese direc block.
void first_write(osFile* file);

// Libera la memoria de file
void file_destroy(osFile* file);


/* -------- Declaración de funciones internas de la API --------- */

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

// Retorna 0 si path es un archivo, 1 si es un directorio, -1 si no existe
int is_dir(char* path);

// Guarda en path_array el array de strings con los nombres de cada nivel del path.
// (Cada string del path queda en el heap asi que hay que usar free_array() al terminar)
void parse_path(char* path, char** path_array);

// Retorna el largo (depth) del path.
short path_length(char* path);

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

// Guarda en parent el string con el path hasta el penultimo nivel, se asume que path no es "/"
void get_parent(char* path, char* parent);

// Retorna un numero de bloque libre y lo marca como usado en el bitmap.
unsigned int get_free_block();

// Pasa num a big endian y lo guarda en buffer de bytes bytes.
void to_big_endian(unsigned char* buffer, unsigned long long num, int bytes);

// Pasa buffer en big endian a little endian y retorna el numero.
unsigned long long to_little_endian(unsigned char* buffer, int bytes);

// Acutaliza el bitmap del block con el valor value (1 o 0)
void update_bitmap(unsigned int block, int value);

// Va al bitmap y libera el bit del bloque (no puede ser 0)
void free_block(unsigned int block);

// Retorna el numero de bloque en que esta el bloque de name 
// (dir o indice), el cual esta dentro de la carpeta de bloque bloque_dir
unsigned int get_block_in_dir(char* name, unsigned int bloque_dir);

// Carga un archivo o carpeta de nuestro pc (orig) al disco (dest). Se usan paths
// con "/" ojala relativos.
void load_to(char* orig, char* dest);

// Retorna 1 si path es valido, 0 si no (empieza con / y termina sin / o es "/")
int valid_path(char* path);


/* -------- Definicion de Funciones expuestas de la API --------- */

void os_mount(char* diskname){
    DISK_NAME = malloc(strlen(diskname) + 1); 
    strcpy(DISK_NAME, diskname);
}
void os_unmount(){
    free(DISK_NAME);
}

void os_bitmap(unsigned int num, short hex){
    if (num > 64) {fprintf(stderr, "El numero sale del rango permitido {0, 64}\n"); return;}
    if (num) print_block(num, hex);
    else print_bitmap(hex);
}

int os_exists(char* path){
    if (!valid_path(path)) return 0; 
    if (is_dir(path) == -1) return 0;
    return 1;
}

void os_ls(char* path){
    if (!valid_path(path) || is_dir(path) != 1) {
        fprintf(stderr, "path inválido\n");
        return;
    }
    unsigned int block = get_block(path);
    printf("Listando: %s\n", path);
    list_dir(block);
}

osFile* os_open(char* path, char mode){
    if (!valid_path(path)) {
        fprintf(stderr, "path inválido\n");
        return 0;
    }
    if (mode != 'r' && mode != 'w') {fprintf(stderr, "Modo debe ser r o w\n"); return NULL;}
    int isdir = is_dir(path);
    if (isdir == -1 && mode == 'r') {fprintf(stderr, "Archivo no existe\n"); return NULL;}
    if (isdir == 0 && mode == 'w') {fprintf(stderr, "Archivo ya existe\n"); return NULL;}
    if (isdir == 1) {fprintf(stderr, "Path no es un archivo\n"); return NULL;}

    char parent[strlen(path)];
    get_parent(path, parent);
    if (mode == 'w' && is_dir(parent) != 1) {fprintf(stderr, "Ruta invalida\n"); return NULL;}
    
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
        buffer[0] = (unsigned char) 1;
        to_big_endian(&buffer[1], file->size, 7);
        write_block(buffer, file->base_index_block);
    }
    file_destroy(file);
}

int os_rm(char* path){
    if (!valid_path(path) || is_dir(path) != 0) {
        fprintf(stderr, "path inválido\n");
        return 0;
    }
    unsigned int index_block = get_block(path);
    unsigned char buffer[2048];
    read_block(buffer, index_block);
    if (buffer[0] == 1) {
        remove_file(path);
    }
    else {
        buffer[0] --;
        write_block(buffer, index_block);
    }
    remove_entrance(path);
    return 0;
}

int os_hardlink(char* orig, char* dest){
    if (!valid_path(orig) || is_dir(orig) != 0 || !valid_path(dest) || is_dir(dest) != -1) {
        fprintf(stderr, "path inválido\n");
        return 0;
    }
    // orig y dest son path validos
    // dest no existe
    // orig es un archivo (o quizas tambien un dir?) -> no, pq los dir no tienen dd guardar el n° de hl
    // Crear entrada para dest (si es que hay espacio en su dir)
    // get block de orig y agregarlo a la entrada de dest
    // aumentar las referencias en el bloque indice de orig
    int path_len = path_length(dest);
    char* path_array[path_len];
    parse_path(dest, path_array);
    char parent[strlen(dest)];
    get_parent(dest, parent);
    char* name = path_array[path_len - 1];

    unsigned int orig_block = get_block(orig);
    create_entrance(parent, name, 1, orig_block);

    unsigned char buffer[2048];
    read_block(buffer, orig_block);
    buffer[0] ++;
    write_block(buffer, orig_block);
    free_array(path_array, path_len);
    printf("Hardlink creado\n");
    return 0;
}

int os_mkdir(char* path){
    if (!valid_path(path) || is_dir(path) != -1) {
        fprintf(stderr, "path inválido\n");
        return 0;
    }
    // path valido
    // existe el path hasta un nivel antes y es dentro de un dir
    // Crear entrada para el dir si hay espacio
    // Y asignarle un bloque libre (00000) para sus entradas
    int path_len = path_length(path);
    char* path_array[path_len];
    parse_path(path, path_array);
    char parent[strlen(path)];
    get_parent(path, parent);
    char* name = path_array[path_len - 1];
    
    unsigned int free_block = get_free_block();
    unsigned char zero_buf[2048] = {0};
    write_block(zero_buf, free_block); // El bloque de dir tiene q estar en 0 
    create_entrance(parent, name, 2, free_block);
    free_array(path_array, path_len);
    return 0;
}

int empty_dir(char* path){
    unsigned int dir_block = get_block(path);
    unsigned char buffer[2048];
    read_block(buffer, dir_block);
    for (int i = 0; i < 64; i++) if (buffer[32 * i] & 192) return 0;
    return 1;
}

int os_rmdir(char* path, short recursive){
    if (!valid_path(path) || is_dir(path) != 1) {
        fprintf(stderr, "path inválido\n");
        return 0;
    }
    // path valido 
    // path es un dir
    // si esta vacio eliminarlo
    // si no y recursive, eliminar todo recursivamente y luego eliminarlo
    unsigned int dir_block = get_block(path);
    if (empty_dir(path)) {
        remove_entrance(path);
        free_block(dir_block);
    }
    else if (recursive) {
        unsigned char buffer[2048];
        read_block(buffer, dir_block);
        for (int i = 0; i < 64; i++)
        {
            if (buffer[32 * i] & 192) {
                char new_path[strlen(path) + strlen((char*)&buffer[32 * i + 3]) + 2];
                if (!strcmp(path, "/")){
                    new_path[0] = '/';
                    strcpy(&new_path[1], (char*)&buffer[32 * i + 3]);
                }
                else {
                    strcpy(new_path, path);
                    new_path[strlen(path)] = '/';
                    strcpy(&new_path[strlen(path) + 1], (char*)&buffer[32 * i + 3]);
                }
                int _is_dir = is_dir(new_path);
                if (_is_dir == 1) os_rmdir(new_path, 1);
                if (_is_dir == 0) os_rm(new_path);
            }
        }
        remove_entrance(path);
        free_block(dir_block);
    }
    return 1;
}

int os_unload(char* orig, char* dest){
    if (!valid_path(orig) || !os_exists(orig)) {
        fprintf(stderr, "path inválido\n");
        return 0;
    }
    // Ver si orig existe
    // Ver si es archivo o carpeta (ambos)
    // dest tiene q ser un path valido hasta el penultimo nivel (sino mkdir no crea el dir)
    // dest no puede ser "/"
    int _is_dir = is_dir(orig);
    if (_is_dir == 1){
        // Si es carpeta crear la carpeta en el PC y dps correr os_unload con todo lo que hay dentro
        mkdir(dest, 0777);
        unsigned int bloque_dir = get_block(orig);
        unsigned char buf[2048];
        read_block(buf, bloque_dir);
        for (int i = 0; i < 64; i++)
        {
            unsigned char mask = 192;
            if (buf[32 * i] & mask) {
                char name[strlen((char*)&buf[32 * i + 3]) + 1];
                strcpy(name, (char*)&buf[32 * i + 3]);

                char new_orig[strlen(orig) + strlen(name) + 2];
                if (!strcmp(orig, "/")) {
                    new_orig[0] = '/';
                    strcpy(&new_orig[1], name);
                }
                else {
                    strcpy(new_orig, orig);
                    new_orig[strlen(orig)] = '/';
                    strcpy(&new_orig[strlen(orig) + 1], name);
                }
                char new_dest[strlen(dest) + strlen(name) + 2];
                strcpy(new_dest, dest); 
                new_dest[strlen(dest)] = '/';
                strcpy(&new_dest[strlen(dest) + 1], name);
                os_unload(new_orig, new_dest);
            }
        }
    }
    else if (_is_dir == 0) {
        // Si es archivo escribirlo en nuestro pc
        osFile* source = os_open(orig, 'r');
        FILE* file = fopen(dest, "wb");
        unsigned char buffer[4096];
        printf("Traspasando %s de tamaño: %lld\n", orig, source->size);
        int read;
        // Leemos hasta que se acaba el archivo
        while ((read = os_read(source, buffer, 4096))) fwrite(buffer, read, 1, file);
        fclose(file);
        os_close(source);
    }
    else printf("Archivo o directorio no existe\n");    
    return 0;
}

int os_load(char* orig){
    if (!os_exists(orig)) {
        fprintf(stderr, "%s ya existe\n", orig);
        return 0;
    }
    // orig no puede ser "/"
    // Ejecutar load_to(orig, "/" + orig[-1])
    int path_len = path_length(orig);
    char* path_array[path_len];
    parse_path(orig, path_array);
    char* name = path_array[path_len - 1];

    char dest[strlen(name) + 2];
    dest[0] = '/';
    strcpy(&dest[1], name);
    load_to(orig, dest);
    free_array(path_array, path_len);
    return 0;
}

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

void remove_file(char* path){
    printf("Borrando %s\n", path);
    osFile* file = osFile_init(path, 'r');
    unsigned int n_data_blocks = ceil((double)file->size / 2048);
    unsigned int n_direc_blocks = ceil((double)n_data_blocks / 512);
    unsigned int n_index_blocks = 1;
    int aux = n_direc_blocks - 509;
    while (aux > 0) {
        n_index_blocks ++; aux -= 511;
    }
    unsigned int index_freed = 0, direc_freed = 0, data_freed = 0;
    unsigned int index_block = file->index_block;
    unsigned int* direc_blocks = file->direc_blocks;
    unsigned int* data_blocks = file->data_blocks;
    for (int index = 0; index < n_index_blocks; index++)
    {
        free_block(index_block); index_freed ++;
        int start = (index == 0)? 2 : 0;
        for (int direc = start; direc < 511; direc++)
        {
            free_block(direc_blocks[direc]); direc_freed ++;
            for (int data = 0; data < 512; data++)
            {
                free_block(data_blocks[data]); data_freed ++;
                if (data_freed == n_data_blocks) break;
            }
            if (direc_freed == n_direc_blocks) break;
            if (direc < 510) read_pointers_block(data_blocks, direc_blocks[direc + 1]);
            else
            {
                index_block = direc_blocks[511];
                read_pointers_block(direc_blocks, index_block);
                read_pointers_block(data_blocks, direc_blocks[0]);
            }
        }
        if (index_freed == n_index_blocks) break;
    }
    printf("Se liberaron %lld bytes\n", file->size);
    os_close(file);
}


// Definicion de funciones internas de la API

void read_block(void* buffer, unsigned int num){
    FILE *file = fopen(DISK_NAME, "rb");
    fseek(file, num * 2048, SEEK_SET);
    fread(buffer, 2048, 1, file);
    fclose(file);
}

void write_block(void* buffer, unsigned int num){
    FILE *file = fopen(DISK_NAME, "r+b");
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
    if (!num) return;
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
    for (unsigned int i = 1; i < 65; i++)
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
    char* name = path_array[path_len - 1];
    create_entrance(parent, name, 1, block);
    free_array(path_array, path_len);
    printf("Archivo creado %s\n", path);
}

void create_entrance(char* parent, char* name, int tipo, unsigned int block){
    unsigned int parent_block = get_block(parent);
    unsigned char buffer[2048];
    read_block(buffer, parent_block);
    for (int i = 0; i < 64; i++)
    {
        unsigned char mask = 192;
        if (!(buffer[32 * i] & mask)) {
            unsigned char buf[3];
            to_big_endian(buf, block, 3);
            buf[0] = buf[0] | ((unsigned char)tipo << 6);
            memcpy(&buffer[32 * i], buf, 3);
            strcpy((char*)&buffer[32 * i + 3], name);
            write_block(buffer, parent_block);
            break;
        }
    }
}

void get_parent(char* path, char* parent){
    // TODO arreglar esta wea, agrear free array
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
    free_array(path_array, path_len);
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
                for (int bit = 0; bit < 8; bit++)
                {
                    unsigned char mask = 1 << bit;
                    if (!(buffer[byte] & mask)){
                        // printf("block: %d, byte: %d, bit: %d\n", block, byte, bit);
                        free_block = ((block - 1) << 14) | (byte << 3) | (7 - bit);
                        // printf("Free Block: %u\n", free_block);
                        // printf("%x \n", buffer[byte]);
                        buffer[byte] = buffer[byte] | mask;
                        // printf("%x \n", buffer[byte]);
                        write_block(buffer, block);
                        return free_block;
                    }
                }
            }
        }
    }
    return 0;
}

void to_big_endian(unsigned char* buffer, unsigned long long num, int bytes){
    for (int i = 0; i < bytes; i++) {
        buffer[i] = (unsigned char) ((num & (255 << ((bytes - i - 1) * 8))) >> ((bytes - i - 1) * 8));
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
    printf("\n");
}

int is_dir(char* path){
    if (!strcmp(path, "/")) return 1;
    
    int path_len = path_length(path);
    char* path_array[path_len];
    parse_path(path, path_array);
    char* name = path_array[path_len - 1];

    char parent[strlen(path)];
    get_parent(path, parent);
    unsigned int parent_block = get_block(parent);

    unsigned char buffer[2048];
    read_block(buffer, parent_block);
    for (int i = 0; i < 64; i++)
    {
        unsigned char code = (buffer[32 * i] & (unsigned char) 192) >> 6;
        if (code && !strcmp((char*)&buffer[32 * i + 3], name)) {
            free_array(path_array, path_len);
            if (code == 1) return 0;
            if (code == 2) return 1;
        }
    }
    free_array(path_array, path_len);
    return -1;
}

void free_block(unsigned int block){
    if (block) update_bitmap(block, 0);
}

void update_bitmap(unsigned int block, int value){
    int bitmap_block = block / (2048 * 8) + 1;
    int byte = (block % (2048 * 8)) / 8;
    int bit = (block % (2048 * 8)) % 8;
    unsigned char buffer[2048];
    read_block(buffer, bitmap_block);
    if (value == 1) buffer[byte] = buffer[byte] | (0x80 >> bit);
    else if (value == 0) buffer[byte] = buffer[byte] & ~(buffer[byte] & (0x80 >> bit));
    write_block(buffer, bitmap_block);
}

void remove_entrance(char* path){
    if (!strcmp(path, "/")) return;
    
    int path_len = path_length(path);
    char* path_array[path_len];
    parse_path(path, path_array);
    char* name = path_array[path_len - 1];
    char parent[strlen(path)];
    get_parent(path, parent);

    unsigned int block_dir = get_block(parent);
    unsigned char buffer[2048];
    read_block(buffer, block_dir);
    for (int i = 0; i < 64; i++)
    {
        unsigned char mask = 192;
        if ((buffer[32 * i] & mask) && !strcmp(name, (char*)&buffer[32 * i + 3] )) {
            buffer[32 * i] = 0;
            write_block(buffer, block_dir);
            break;
        }
    }
    free_array(path_array, path_len);
}

void load_to(char* orig, char* dest){
    // Revisar si orig es archivo o carpeta
    struct stat sb;
    stat(orig, &sb);
    if (S_ISDIR(sb.st_mode)) {
        // Si es carpeta hacer load_to(orig/x, dest/x) de todo lo que hay dentro 
        // hay que crear dest, ya que se asume que es carpeta nueva
        os_mkdir(dest);
        DIR* parent_dir = opendir(orig);
        struct dirent* dir;
        while ((dir = readdir(parent_dir)))
        {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;
            char new_orig[strlen(orig) + strlen(dir->d_name) + 2];
            strcpy(new_orig, orig);
            new_orig[strlen(orig)] = '/';
            strcpy(&new_orig[strlen(orig) + 1], dir->d_name);

            char new_dest[strlen(dest) + strlen(dir->d_name) + 2];
            if (!strcmp(dest, "/")){
                new_dest[0] = '/';
                strcpy(&new_dest[1], dir->d_name);
            }
            else {
                strcpy(new_dest, dest);
                new_dest[strlen(dest)] = '/';
                strcpy(&new_dest[strlen(dest) + 1], dir->d_name);
            }
            load_to(new_orig, new_dest);                        
        }
    }
    else if (S_ISREG(sb.st_mode)) {
        // Si es archivo cargarlo al disco
        FILE* source = fopen(orig, "rb");
        osFile* file = os_open(dest, 'w');
        unsigned char buffer[4096];
        long blocks_read = 0;
        while (1) // Leemos bloques de 4096 bytes del archivo hasta que no podamos mas
        {
            if (!fread(buffer, 4096, 1, source)) break;
            blocks_read ++;
            os_write(file, buffer, 4096);
        }
        // Una vez q no queden bloques leemos los bytes < 4096 que queden en la "cola" de source
        // Hacemos esto por performance para no tener que leer  tantos bloques de 1 byte
        fseek(source, 4096 * blocks_read, SEEK_SET);
        int restantes = fread(buffer, 1, 4096, source);
        os_write(file, buffer, restantes);
        fclose(source);
        os_close(file);
    }
    else
    {
        printf("Archivo o carpeta no funca\n");
    }
}

int valid_path(char* path){
    if (!strcmp(path, "/")) return 1;
    if (path[0] != '/') return 0;
    if (path[strlen(path) - 1] == '/') return 0;
    return 1;
}