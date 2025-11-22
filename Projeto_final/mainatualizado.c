#include <raylib.h>
#include <stdio.h>
#include <math.h>

// Dimensões do mapa em "blocos"
#define MAP_LINES 20
#define MAP_COLS  80
// Tamanho de cada bloco em pixels na tela
#define BLOCK_SIZE 20
#define HUD_HEIGHT 100 //A HUD EH A AREA PRETA EMBAIXO
// Tamanho da tela (calculado a partir do mapa)
#define SCREEN_WIDTH (MAP_COLS * BLOCK_SIZE)
#define SCREEN_HEIGHT ((MAP_LINES * BLOCK_SIZE) + HUD_HEIGHT)
#define VELOCIDADE_MAX 4.0f
#define ACELERACAO 0.1f
#define ROTACAO 3.0f //sao 3 graus por frame pq em 1 segundo damos meia volta (60(fps) * 3) =  180
#define ATRITO 0.97f


// Variável global ou passada como parâmetro
char g_mapa[MAP_LINES][MAP_COLS];

//struct do carro
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

void CarregarMapa(char *fileNome) {
    FILE *arquivo = fopen(fileNome, "r");
    if (arquivo == NULL) {
        TraceLog(LOG_ERROR, "Nao foi possivel abrir o arquivo do mapa: %s", fileNome);
        return;
    }

    // LIMPEZA: Garante que o mapa começa vazio (tudo cinza/pista)

    for (int i = 0; i < MAP_LINES; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            g_mapa[i][j] = ' ';
        }
    }

    // Leitura letra por letra
    int linha = 0;
    int coluna = 0;
    char c;

    // Enquanto houver letras no arquivo...
    while ((c = fgetc(arquivo)) != EOF) {

        // Se chegamos no fim da linha (Enter), pula para a próxima linha da matriz
        if (c == '\n') {
            linha++;
            coluna = 0;
            continue; // Pula para a próxima letra do arquivo
        }

        // Se o arquivo for maior que a matriz, ignora o excesso
        if (linha >= MAP_LINES) break;
        if (coluna >= MAP_COLS) {
            // Se a linha do texto for muito longa, ignora o resto dela
            continue;
        }

        // Ignora o caractere '\r' (lixo do Windows)
        if (c == '\r') continue;

        // --- LÓGICA DE PREENCHIMENTO ---
        if (c == 'S') {
            InicializarJogador(coluna * BLOCK_SIZE, linha * BLOCK_SIZE);
            g_mapa[linha][coluna] = ' '; // O 'S' vira chão
        }
        else if (c == 'E') {
            g_inimigo.posicao = (Vector2){ (float)coluna * BLOCK_SIZE, (float)linha * BLOCK_SIZE };
            g_inimigo.angulo = -90.0f;
            g_mapa[linha][coluna] = ' '; // O 'E' vira chão
        }
        else {
            // Copia 'p', 'L' ou ' ' para a matriz
            g_mapa[linha][coluna] = c;
        }

        // Vai para a próxima coluna
        coluna++;
    }

    fclose(arquivo);
}
void DesenhaMapa(void)
{
    for (int i = 0; i < MAP_LINES; i++)
    {
        for (int j = 0; j < MAP_COLS; j++)
        {

            int posX = j * BLOCK_SIZE;
            int posY = i * BLOCK_SIZE;

            // cor que define um "ERRO"
            Color cor = (Color)
            {
                190, 190, 150, 255
            }; // Se não for nada, pinte de cinza
            switch (g_mapa[i][j]) // eh como se pegasse o caracter que esta na linha i e coluna j e aplicasse os cases
            {
            case 'p':
                cor = (Color)
                {
                    139, 69, 19, 255
                }; //especificamos a tonalidade do marrom
                break;

            case ' ':
                cor = (Color)
                {
                    190, 190, 150, 255
                }; //especificamos a tonalidade do cinza
                break;
            case 'L':
                cor = BLACK;
                break;

            }

            DrawRectangle(posX, posY, BLOCK_SIZE, BLOCK_SIZE, cor); // funcao que desenha, dentro tem (pos x, pos y, largura, altura, cor)
        }
    }
}

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
void DesenharJogador(void)
{
    // Retângulo do carro com rotação
    Rectangle retanguloCarro = {g_jogador.posicao.x + BLOCK_SIZE/2, g_jogador.posicao.y + BLOCK_SIZE/2, BLOCK_SIZE, BLOCK_SIZE };
    Vector2 origem = { BLOCK_SIZE/2, BLOCK_SIZE/2 }; //define a origem como o centro do bloco

    // Desenha o retângulo vermelho girando
    DrawRectanglePro(retanguloCarro, origem, g_jogador.angulo, RED);
}



int main(void)
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Rock N Roll Racing"); //funcao que inicializa a janela, dentro tem (largura , altura, titulo)
    SetTargetFPS(60); // diz pro pc rodar em 60 fps, ou pelo menos tentar, caso ele n tenha essa capacidade

    // 2. Carregar dados (fora do loop)
    CarregarMapa("pista1.txt");

    // 3. Game Loop
    while (!WindowShouldClose()) // Rodar o jogo enquanto a janela NAO fechar
    {
        AtualizarPosicao(); //chama a funcao de movimentacao do carro

        BeginDrawing(); // inicializa o desenho
        ClearBackground(BLACK); // "limpa" o rastro que seria deixado, entao exclui uma imagem anterior para fazer a proxima

        DesenhaMapa(); // Chama a função de desenho do mapa
        DesenharJogador(); //Chama a função que desenha o carro

        EndDrawing(); // finaliza o desenho
    }

    // 6. Finalização
    CloseWindow();
    return 0;
}

