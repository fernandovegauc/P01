typedef struct osFile {
    unsigned int index_block;
    int n_index_block; // El numero de index block en que esta el cursor (0 el principal)
    unsigned int dir_blocks[512]; // El ultimo apunta al siguiente index block
    int n_dir_block; // El numero de dir block en que está el cursor
    unsigned int data_blocks[512]; 
    int n_data_block; // El numero de data block en que está el cursor

    unsigned int virtual_cursor; // Cursor respecto al inicio del archivo
    unsigned int relative_cursor; // Cursor respecto al data block (0:2047)

    unsigned int size;
    char mode;
    char name[29];
} osFile;

// Variable global definida en os_API.c 
extern char* DISK_NAME;


// Funciones generales

// Asignar la variable global con el nombre del disco.
void os_mount(char* diskname);
// Liberar la variable global
void os_unmount();

// Imprime por stderr el bloque num (1..64) del bitman. 
// hex es un bool que indica si se imprime en hexadecimal.
// Si num=0 se imprime todo el bitmap.
void os_bitmap(unsigned short num, short hex);

// Retorna 1 si existe el archivo en path.
int os_exists(char* path);

// Lista los elementos de la carpeta path (no recusriva).
void os_ls(char* path);


// Funciones de manejo de archivos.

// Retorna un nuevo osFile abriendo el archivo path.
osFile* os_open(char* path, char mode);

// Lee nbytes de osFIle y los guarda en buffer.
// Sigue la estructura de bloques del disco.
int os_read(osFile* file_desc, void* buffer, int nbytes);

// Escribe nbytes del buffer en el archivo.
int os_write(osFile* file_desc, void* buffer, int nbytes);

// Cierra el archivo y escribe de forma permanente los cambios.
int os_close(osFile* file_desc);

// Elimina la referencia al archivo. Si es la unica referencia elimina 
// el archivo.
int os_rm(char* path);

// Crea una nueva referenecia (hardlink) al archivo dest.
int os_hardlink(char* orig, char* dest);

// Crea un nuevo directorio.
int os_mkdir(char* path);

// Elimina un directorio vacio. Puede ser de forma recursiva si no esta vacio. 
int os_rmdir(char* path, short recursive);

// "Descarga" el archivo dest a nuestro pc.
int os_unload(char* orig, char* dest);

// Cargamos un archivo o carpeta a nuestra root folder del disco.
int os_load(char* orig);
