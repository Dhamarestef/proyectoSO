#ifndef INTERFAZ_NCURSES_H
#define INTERFAZ_NCURSES_H

#include <ncurses.h>
#include <string.h>
#include "Operaciones.h"

// Filas fijas de la pantalla (layout minimalista, sin bordes ni color)
#define FILA_TITULO     0
#define FILA_PROMPT     2
#define FILA_CABECERA   5
#define FILA_LINEA      6
#define FILA_RESULTADO  7
#define FILA_MENSAJE    10
#define FILA_AYUDA      13

// Sangría común de la tabla
#define INDENT "  "

// Ancho de la columna Status ("ERROR operando invalido" mide 23 caracteres)
#define ANCHO_STATUS 26

// Tiempos de la animación (milisegundos)
#define MS_BLANCO   150   // tiempo que la línea permanece borrada
#define MS_VISIBLE  700   // tiempo que la línea permanece visible

// ID del proceso: aumenta 1 por cada archivo ejecutado (persiste entre ejecuciones)
static int contador_procesos = 0;

// Imprime la cabecera de la tabla y su línea separadora (quedan fijas en pantalla)
void imprimir_cabecera_ncurses() {
    move(FILA_CABECERA, 0);
    clrtoeol();
    mvprintw(FILA_CABECERA, 0, INDENT "%-6s%-6s%-6s%-6s%-6s%-6s%-18s%-*s%s",
             "ID", "PC", "AX", "BX", "CX", "DX", "IR",
             ANCHO_STATUS, "Status", "Nombre");

    move(FILA_LINEA, 0);
    clrtoeol();
    mvprintw(FILA_LINEA, 0, INDENT "------------------------------------------------------------------------------------------");
    refresh();
}

// Muestra un mensaje de una línea (errores, avisos, etc.), o lo limpia si mensaje es ""
void mostrar_mensaje(const char *mensaje) {
    move(FILA_MENSAJE, 0);
    clrtoeol();
    mvprintw(FILA_MENSAJE, 0, INDENT "%s", mensaje);
    refresh();
}

// Borra la fila de resultado (deja el espacio en blanco antes de la siguiente línea)
void borrar_fila_resultado() {
    move(FILA_RESULTADO, 0);
    clrtoeol();
    refresh();
    napms(MS_BLANCO);
}

// Imprime una fila de resultado en el mismo lugar donde estaba la anterior
void imprimir_fila_resultado(int id, CPU *cpu, const char *nombre) {
    mvprintw(FILA_RESULTADO, 0, INDENT "%-6d%-6d%-6d%-6d%-6d%-6d%-18s%-*s%s",
             id, cpu->pc, cpu->ax, cpu->bx, cpu->cx, cpu->dx, cpu->ir,
             ANCHO_STATUS, cpu->status, nombre);
    refresh();
    napms(MS_VISIBLE);
}

// Ejecuta el archivo mostrando cada resultado en el mismo lugar, uno a la vez
void ejecutar_archivo_ncurses(const char *archivo) {
    FILE *file = fopen(archivo, "r");
    if (file == NULL) {
        mostrar_mensaje("proceso no encontrado");
        return;
    }

    CPU mi_cpu = {0, 0, 0, 0, 1, "", ""}; // PC inicia en 1
    char linea[100];
    int id = ++contador_procesos;         // cada archivo ejecutado suma 1

    mostrar_mensaje(""); // limpia mensajes previos
    imprimir_cabecera_ncurses();
    move(FILA_RESULTADO, 0);
    clrtoeol();
    refresh();

    while (fgets(linea, sizeof(linea), file)) {
        procesar_instruccion(&mi_cpu, linea);

        // 1) Se borra lo que había en la fila de resultado
        borrar_fila_resultado();

        // 2) Se imprime la nueva línea en ese mismo lugar
        imprimir_fila_resultado(id, &mi_cpu, archivo);

        mi_cpu.pc++;
    }

    if (fclose(file) != 0) {
        mostrar_mensaje("Advertencia: error al cerrar el archivo");
    } else {
        mostrar_mensaje("Ejecucion terminada. Presiona una tecla para continuar...");
        nodelay(stdscr, FALSE);
        getch();
        mostrar_mensaje("");
    }
}

// Lee una línea de comando desde la fila de prompt
void leer_comando(char *destino, int tam) {
    move(FILA_PROMPT, 0);
    clrtoeol();
    mvprintw(FILA_PROMPT, 0, "> ");
    refresh();

    echo();
    curs_set(1);
    getnstr(destino, tam - 1);
    noecho();
    curs_set(0);
}

// Bucle principal de la interfaz ncurses
void iniciar_consola_ncurses() {
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    noecho();

    mvprintw(FILA_TITULO, 0, "Simulador de CPU");
    mvprintw(FILA_AYUDA, 0, INDENT "ejecutar <archivo>   salir");
    refresh();

    char entrada[100];
    char comando[100];
    char archivo[100];

    while (1) {
        leer_comando(entrada, sizeof(entrada));

        char *token = strtok(entrada, " ");
        if (token == NULL) continue;
        strncpy(comando, token, sizeof(comando) - 1);
        comando[sizeof(comando) - 1] = '\0';

        if (strcmp(comando, "salir") == 0) {
            break;
        }
        else if (strcmp(comando, "ejecutar") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                strncpy(archivo, token, sizeof(archivo) - 1);
                archivo[sizeof(archivo) - 1] = '\0';

                if (strtok(NULL, " ") != NULL) {
                    mostrar_mensaje("Comando invalido");
                } else {
                    ejecutar_archivo_ncurses(archivo);
                }
            } else {
                mostrar_mensaje("Falta el nombre del archivo");
            }
        } else {
            mostrar_mensaje("Comando invalido");
        }
    }

    endwin();
}

#endif