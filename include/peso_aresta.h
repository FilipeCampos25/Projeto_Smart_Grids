#ifndef PESO_ARESTA_H
#define PESO_ARESTA_H

/* A existencia da aresta e independente do peso. Assim, uma aresta existente
 * pode nao ter peso informado, enquanto zero continua sendo um peso valido. */
typedef enum {
    ORIGEM_PESO_NAO_INFORMADA = 0,
    ORIGEM_PESO_COMPRIMENTO_MEDIDO,
    ORIGEM_PESO_CUSTO_ASSUMIDO
} OrigemPesoAresta;

typedef struct {
    int possui_peso;
    double valor;
    OrigemPesoAresta origem;
} PesoAresta;

#endif
