#ifndef OPERACIONES_H
#define OPERACIONES_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

typedef struct {
    int ax, bx, cx, dx;
    int pc;
    char ir[100];
    char status[50];
} CPU;

int es_mayuscula(const char *cadena) {
    for (int i = 0; cadena[i] != '\0'; i++) {
        if (isalpha((unsigned char)cadena[i]) && islower((unsigned char)cadena[i])) {
            return 0;
        }
    }
    return 1;
}

// NUEVO: valida que una cadena sea el nombre de un registro (para operandos destino)
int es_registro(const char *operando) {
    return (strcmp(operando, "AX") == 0 || strcmp(operando, "BX") == 0 ||
            strcmp(operando, "CX") == 0 || strcmp(operando, "DX") == 0);
}

// NUEVO: valida un entero (signo opcional + solo dígitos) y detecta overflow de int
int es_numero_valido(const char *operando, long *valor_out) {
    if (operando[0] == '\0') return 0;

    int i = 0;
    if (operando[0] == '-' || operando[0] == '+') i = 1;

    if (operando[i] == '\0') return 0; // "-" o "+" solo, sin dígitos (bug original)

    for (; operando[i] != '\0'; i++) {
        if (!isdigit((unsigned char)operando[i])) return 0;
    }

    errno = 0;
    char *fin;
    long valor = strtol(operando, &fin, 10);
    if (errno == ERANGE || valor > INT_MAX || valor < INT_MIN) return 0; // overflow

    *valor_out = valor;
    return 1;
}

int obtener_valor(CPU *cpu, char *operando, int *es_valido) {
    *es_valido = 1;
    if (strcmp(operando, "AX") == 0) return cpu->ax;
    if (strcmp(operando, "BX") == 0) return cpu->bx;
    if (strcmp(operando, "CX") == 0) return cpu->cx;
    if (strcmp(operando, "DX") == 0) return cpu->dx;

    long valor;
    if (!es_numero_valido(operando, &valor)) {
        *es_valido = 0;
        return 0;
    }
    return (int)valor;
}

void guardar_valor(CPU *cpu, char *registro, int valor) {
    if (strcmp(registro, "AX") == 0) cpu->ax = valor;
    else if (strcmp(registro, "BX") == 0) cpu->bx = valor;
    else if (strcmp(registro, "CX") == 0) cpu->cx = valor;
    else if (strcmp(registro, "DX") == 0) cpu->dx = valor;
}

void procesar_instruccion(CPU *cpu, char *instruccion_original) {
    char copia[100];
    // CORREGIDO: strncpy con terminador garantizado en vez de strcpy sin límite
    strncpy(copia, instruccion_original, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0';

    copia[strcspn(copia, "\r\n")] = 0;

    strncpy(cpu->ir, copia, sizeof(cpu->ir) - 1);
    cpu->ir[sizeof(cpu->ir) - 1] = '\0';
    strcpy(cpu->status, "OK");

    if (!es_mayuscula(copia)) {
        strcpy(cpu->status, "ERROR de sintaxis");
        return;
    }

    char *comando = strtok(copia, " ,");
    char *op1 = strtok(NULL, " ,");
    char *op2 = strtok(NULL, " ,");
    char *extra = strtok(NULL, " ,");

    if (comando == NULL) return; // Línea en blanco

    if (extra != NULL) {
        strcpy(cpu->status, "ERROR de sintaxis");
        return;
    }

    int valido1 = 1, valido2 = 1;

    // Instrucciones de 1 parámetro (INC, DEC)
    if (strcmp(comando, "INC") == 0 || strcmp(comando, "DEC") == 0) {
        if (op1 == NULL || op2 != NULL) {
            strcpy(cpu->status, "ERROR Sintaxis");
            return;
        }
        // CORREGIDO: op1 debe ser un registro, no un número (antes se aceptaba "INC 7" sin error)
        if (!es_registro(op1)) {
            strcpy(cpu->status, "ERROR operando invalido");
            return;
        }
        int val = obtener_valor(cpu, op1, &valido1);
        if (strcmp(comando, "INC") == 0) guardar_valor(cpu, op1, val + 1);
        if (strcmp(comando, "DEC") == 0) guardar_valor(cpu, op1, val - 1);
    }
    // Instrucciones de 2 parámetros (MOV, ADD, SUB, MUL, DIV)
    else if (strcmp(comando, "MOV") == 0 || strcmp(comando, "ADD") == 0 ||
             strcmp(comando, "SUB") == 0 || strcmp(comando, "MUL") == 0 ||
             strcmp(comando, "DIV") == 0) {

        if (op1 == NULL || op2 == NULL) {
            strcpy(cpu->status, "ERROR falta operando");
            return;
        }

        // CORREGIDO: op1 (destino) debe ser un registro
        if (!es_registro(op1)) {
            strcpy(cpu->status, "ERROR operando invalido");
            return;
        }

        int val2 = obtener_valor(cpu, op2, &valido2);
        if (!valido2) {
            strcpy(cpu->status, "ERROR OP invalido");
            return;
        }

        int val1 = obtener_valor(cpu, op1, &valido1);
        // CORREGIDO: antes valido1 se calculaba pero nunca se revisaba
        if (!valido1) {
            strcpy(cpu->status, "ERROR OP invalido");
            return;
        }

        if (strcmp(comando, "MOV") == 0) guardar_valor(cpu, op1, val2);
        else if (strcmp(comando, "ADD") == 0) guardar_valor(cpu, op1, val1 + val2);
        else if (strcmp(comando, "SUB") == 0) guardar_valor(cpu, op1, val1 - val2);
        else if (strcmp(comando, "MUL") == 0) guardar_valor(cpu, op1, val1 * val2);
        else if (strcmp(comando, "DIV") == 0) {
            if (val2 == 0) {
                strcpy(cpu->status, "ERROR Division por cero");
            } else {
                guardar_valor(cpu, op1, val1 / val2);
            }
        }
    } else {
        strcpy(cpu->status, "ERROR Sintaxis");
    }
}
#endif
