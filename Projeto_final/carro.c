#include <raylib.h>
#include <stdio.h>
#include <math.h>
#define BLOCK_SIZE 20
#define VELOCIDADE_MAX 4.0f
#define ACELERACAO 0.1f
#define ROTACAO 3.0f //sao 3 graus por frame pq em 1 segundo damos meia volta (60(fps) * 3) =  180
#define ATRITO 0.97f

typedef struct Carro
{
    Vector2 posicao;      // Posição (x, y) do carro na tela
    Vector2 velocidade;   // Velocidade (x, y)
    float angulo;         // Direção para onde o carro aponta (em graus)
    float velocidadeLinear;
    int vida;
    int voltas;
    // int inventario(pra depois)
} CARRO;


CARRO g_jogador;
CARRO g_inimigo;
void InicializarJogador(int x, int y);

void AtualizarPosicao(void)
{
    // 1. Rotação (A e D)
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    {
        g_jogador.angulo = g_jogador.angulo - ROTACAO; //girando no sentido antihorario, por isso o menos
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    {
        g_jogador.angulo = g_jogador.angulo + ROTACAO; //girando no sentido horario, por isso soma
    }



    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    {
        g_jogador.velocidadeLinear = g_jogador.velocidadeLinear + ACELERACAO; // aumentando a velocidade quando vai pra cima
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    {
        g_jogador.velocidadeLinear = g_jogador.velocidadeLinear - ACELERACAO; // diminuindo a velocidade quando vai pra cima
    }


    if (g_jogador.velocidadeLinear > VELOCIDADE_MAX)
    {
        g_jogador.velocidadeLinear = VELOCIDADE_MAX; //aplicação de um limite de velociade
    }
    if (g_jogador.velocidadeLinear < -VELOCIDADE_MAX/2)
    {
        g_jogador.velocidadeLinear = -VELOCIDADE_MAX/2; // pra fazer sentido a ré tem que ser mais lenta
    }
    // Aplica atrito (o carro para sozinho se soltar W)
    g_jogador.velocidadeLinear = g_jogador.velocidadeLinear * ATRITO;


    // DEG2RAD é uma constante q converte graus em radianos
    g_jogador.velocidade.x = cos(g_jogador.angulo * DEG2RAD) * g_jogador.velocidadeLinear;
    g_jogador.velocidade.y = sin(g_jogador.angulo * DEG2RAD) * g_jogador.velocidadeLinear;
    //como o computador so entende x e y, dividimos isso em duas velocidade e multiplicamos pela angulacao do seno ou do cosseno

    // 5. Atualiza a Posição
    g_jogador.posicao.x = g_jogador.posicao.x + g_jogador.velocidade.x;
    g_jogador.posicao.y = g_jogador.posicao.y + g_jogador.velocidade.y;

    //Normalizamos para int pq nao existe posicao "pela metade", ou seja, precisamos de um valor inteiro para colcoar na matriz
    int colunaAlvo = (int)((g_jogador.posicao.x + g_jogador.velocidade.x + (BLOCK_SIZE/2))/BLOCK_SIZE);
    //usamos "block_size/2" para fazer um ajuste, ou seja, serve para colocarmos o ponto de verificacao para o centro do bloco
    int linhaAlvo  = (int)((g_jogador.posicao.y + g_jogador.velocidade.y + (BLOCK_SIZE/2))/BLOCK_SIZE);

   bool bateu = false;
    // Verificação de segurança pra não sair do mapa
    if (colunaAlvo < 0 || colunaAlvo >= MAP_COLS || linhaAlvo < 0 || linhaAlvo >= MAP_LINES) {
        bateu = true;
    } else if (g_mapa[linhaAlvo][colunaAlvo] == 'p') {
        bateu = true;
    }

    if (bateu)
    {
        // bateu na parede e então zera sua velocidade
        g_jogador.velocidadeLinear = 0;
    }
    else
    {
        // Agora a posição muda de verdade
        g_jogador.posicao.x = g_jogador.posicao.x + g_jogador.velocidade.x;
        g_jogador.posicao.y = g_jogador.posicao.y + g_jogador.velocidade.y;
    }
}
void InicializarJogador(int x, int y)
{
    g_jogador.posicao = (Vector2)
    {
        (float)x, (float)y
    }; //Pega o valor X e Y, transforma em decimais (float) e salva dentro da posição do jogador de um novo Vector2
    g_jogador.velocidade = (Vector2){0, 0};
    g_jogador.velocidadeLinear = 0.0f;
    g_jogador.angulo = -90.0f; // Aponta para cima
    g_jogador.vida = 100;
    g_jogador.voltas = 0;
}
void DesenharJogador(void) //nessa funcao substituimos pelos sprites certos
{
    // Retângulo do carro com rotação
    Rectangle retanguloCarro = {g_jogador.posicao.x + BLOCK_SIZE/2, g_jogador.posicao.y + BLOCK_SIZE/2, BLOCK_SIZE, BLOCK_SIZE };
    Vector2 origem = { BLOCK_SIZE/2, BLOCK_SIZE/2 }; //define a origem como o centro do bloco

    // Desenha o retângulo vermelho girando
    DrawRectanglePro(retanguloCarro, origem, g_jogador.angulo, RED);
}
