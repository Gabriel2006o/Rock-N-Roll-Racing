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
#define VELOCIDADE_MAX 3.0f
#define VELOCIDADE_MAX_INIMIGO 4.0f
#define ACELERACAO 0.1f
#define ROTACAO 3.0f //sao 3 graus por frame pq em 1 segundo damos meia volta (60(fps) * 3) =  180
#define ATRITO 0.97f


// Variável global ou passada como parâmetro
char g_mapa[MAP_LINES][MAP_COLS];

//struct do carro
typedef struct Carro
{
    Vector2 posicao;      // Posição (x, y)
    Vector2 velocidade;   // Vetor de movimento
    float angulo;         // Ângulo atual (Graus)
    float velocidadeLinear;

    // --- NOVOS CAMPOS ---
    Texture2D sprite;     // A imagem do carro
    float targetRotation; // Para a IA: Onde ela QUER virar
    float velocidadeGiro;       // Para a IA: Velocidade de giro
    Vector2 origem;       // Centro da imagem para rotação

    int vida;             
    int voltas;          
    // int inventario(pra depois)
} CARRO;

CARRO g_jogador;
CARRO g_inimigo;

void InicializarJogador(int x, int y);
void InicializarInimigo(int x, int y);

void CarregarMapa(char* fileNome) {
    FILE* arquivo = fopen(fileNome, "r");
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
            InicializarJogador(coluna * BLOCK_SIZE, linha * BLOCK_SIZE + 13);
            g_mapa[linha][coluna] = ' '; // O 'S' vira chão
        }
        else if (c == 'E') {
            g_inimigo.posicao = (Vector2){ (float)coluna * BLOCK_SIZE, (float)linha * BLOCK_SIZE};
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
void AtualizarInimigo(void)
{
    float dt = GetFrameTime();
    static float tempoSemParede = 0.0f;

    // ==============================================================================
    // 1. VETORES E DISTÂNCIAS
    // ==============================================================================

    float rad = DEG2RAD;

    // Alcance para ativar o modo "Decisão" (frente)
    float alcanceSensorFrente = BLOCK_SIZE * 3.5f;

    // Alcance lateral esquerda (para saber se tem parede ao lado)
    float alcanceSensorLat = BLOCK_SIZE * 3.0f;

    // Alcance do scanner de decisão (10 blocos)
    float alcanceScanDecisao = BLOCK_SIZE *10.0f;

    // --- DIREÇÕES (NEGATIVAS, pois o movimento é invertido) ---
    Vector2 dirFrente = { -cosf(g_inimigo.angulo * rad), -sinf(g_inimigo.angulo * rad) };
    Vector2 dirEsq = { -cosf((g_inimigo.angulo + 90) * rad), -sinf((g_inimigo.angulo + 90) * rad) };
    Vector2 dirDir = { -cosf((g_inimigo.angulo - 90) * rad), -sinf((g_inimigo.angulo - 90) * rad) };

    // ==============================================================================
    // 2. DETECÇÃO
    // ==============================================================================

    bool viuFrente = false;
    bool viuParedeEsq = false;
    float distRealEsq = 9999.0f;

    int c, l;

    // --- Checa colisão à frente ---
    for (float i = 0; i <= alcanceSensorFrente; i += 10.0f) {
        c = (int)((g_inimigo.posicao.x + dirFrente.x * i) / BLOCK_SIZE);
        l = (int)((g_inimigo.posicao.y + dirFrente.y * i) / BLOCK_SIZE);
        if (c < 0 || c >= MAP_COLS || l < 0 || l >= MAP_LINES || g_mapa[l][c] == 'p') {
            viuFrente = true;
            break;
        }
    }

    // --- Checa parede à esquerda ---
    for (float i = 0; i <= alcanceSensorLat; i += 10.0f) {
        c = (int)((g_inimigo.posicao.x + dirEsq.x * i) / BLOCK_SIZE);
        l = (int)((g_inimigo.posicao.y + dirEsq.y * i) / BLOCK_SIZE);
        if (c < 0 || c >= MAP_COLS || l < 0 || l >= MAP_LINES || g_mapa[l][c] == 'p') {
            distRealEsq = i;
            viuParedeEsq = true;
            break;
        }
    }

    // ==============================================================================
    // 3. CÉREBRO DA IA
    // ==============================================================================

    g_inimigo.targetRotation = g_inimigo.angulo; // padrão: seguir reto

    // -----------------------------------------------------------------------
    // CENÁRIO A: OBSTÁCULO À FRENTE (ESCANEANDO 10 BLOCOS)
    // -----------------------------------------------------------------------
    if (viuFrente) {
        tempoSemParede = 0.0f;

        // Scan esquerda
        float espacoLivreEsq = 0.0f;
        for (float i = 0; i <= alcanceScanDecisao; i += 3.0f) {
            c = (int)((g_inimigo.posicao.x + dirEsq.x * i) / BLOCK_SIZE);
            l = (int)((g_inimigo.posicao.y + dirEsq.y * i) / BLOCK_SIZE);
            if (c < 0 || c >= MAP_COLS || l < 0 || l >= MAP_LINES || g_mapa[l][c] == 'p') break;
            espacoLivreEsq = i;
        }

        // Scan direita
        float espacoLivreDir = 0.0f;
        for (float i = 0; i <= alcanceScanDecisao; i += 3.0f) {
            c = (int)((g_inimigo.posicao.x + dirDir.x * i) / BLOCK_SIZE);
            l = (int)((g_inimigo.posicao.y + dirDir.y * i) / BLOCK_SIZE);
            if (c < 0 || c >= MAP_COLS || l < 0 || l >= MAP_LINES || g_mapa[l][c] == 'p') break;
            espacoLivreDir = i;
        }

        // Escolhe lado com mais espaço (SINAIS INVERTIDOS)
        if (espacoLivreEsq > espacoLivreDir) g_inimigo.targetRotation += 4.0f; // virar esquerda
        else g_inimigo.targetRotation -= 4.0f;                                   // virar direita
    }

    // -----------------------------------------------------------------------
    // CENÁRIO B: PAREDE À ESQUERDA (SEGUE RETO)
    // -----------------------------------------------------------------------
    else if (viuParedeEsq) {
        tempoSemParede = 0.0f;

        // Se muito perto da parede, desvia um pouco
        if (distRealEsq < BLOCK_SIZE * 0.8f) {
            g_inimigo.targetRotation -= 1.0f; // pequeno ajuste
        }
        else {
            g_inimigo.targetRotation = g_inimigo.angulo; // segue reto
        }
    }

    // -----------------------------------------------------------------------
    // CENÁRIO C: PERDEU PAREDE / ESQUINA
    // -----------------------------------------------------------------------
    else {
        tempoSemParede += dt;

        // Anda reto por um instante
        if (tempoSemParede < 1.0f) {
            g_inimigo.targetRotation = g_inimigo.angulo;
        }
        else {
            // Depois vira para esquerda em busca da parede
            g_inimigo.targetRotation += 2.5f; // sinal invertido
        }
    }

    // ==============================================================================
    // 4. SUAVIZAÇÃO E FÍSICA
    // ==============================================================================

    // Ajusta diferença de rotação
    float diff = g_inimigo.targetRotation - g_inimigo.angulo;
    while (diff > 180) diff -= 360;
    while (diff < -180) diff += 360;

    float agilidade = g_inimigo.velocidadeGiro;
    if (viuFrente) agilidade *= 2.5f;

    float passoGiro = agilidade * dt * 60.0f;
    if (fabs(diff) < passoGiro) g_inimigo.angulo = g_inimigo.targetRotation;
    else g_inimigo.angulo += (diff > 0 ? passoGiro : -passoGiro);

    // --- Aceleração ---
    float velocidadeAlvo = VELOCIDADE_MAX_INIMIGO;
    if (viuFrente) velocidadeAlvo = VELOCIDADE_MAX_INIMIGO * 0.2f;
    else if (fabs(diff) > 10.0f) velocidadeAlvo = VELOCIDADE_MAX_INIMIGO * 0.7f;

    if (g_inimigo.velocidadeLinear < velocidadeAlvo) {
        g_inimigo.velocidadeLinear += ACELERACAO;
    }
    else {
        g_inimigo.velocidadeLinear *= ATRITO;
    }

    // --- Movimento NEGATIVO ---
    g_inimigo.posicao.x -= cosf(g_inimigo.angulo * rad) * g_inimigo.velocidadeLinear;
    g_inimigo.posicao.y -= sinf(g_inimigo.angulo * rad) * g_inimigo.velocidadeLinear;

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
    if (g_jogador.velocidadeLinear < -VELOCIDADE_MAX / 2)
    {
        g_jogador.velocidadeLinear = -VELOCIDADE_MAX / 2; // pra fazer sentido a ré tem que ser mais lenta
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
    int colunaAlvo = (int)((g_jogador.posicao.x + g_jogador.velocidade.x + (BLOCK_SIZE / 2)) / BLOCK_SIZE);
    //usamos "block_size/2" para fazer um ajuste, ou seja, serve para colocarmos o ponto de verificacao para o centro do bloco
    int linhaAlvo = (int)((g_jogador.posicao.y + g_jogador.velocidade.y + (BLOCK_SIZE / 2)) / BLOCK_SIZE);

    bool bateu = false;
    // Verificação de segurança pra não sair do mapa
    if (colunaAlvo < 0 || colunaAlvo >= MAP_COLS || linhaAlvo < 0 || linhaAlvo >= MAP_LINES) {
        bateu = true;
    }
    else if (g_mapa[linhaAlvo][colunaAlvo] == 'p') {
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
    g_jogador.velocidade = (Vector2){ 0, 0 };
    g_jogador.velocidadeLinear = 0.0f;
    g_jogador.angulo = 180.0f; 
    g_jogador.vida = 100;
    g_jogador.voltas = 0;
}
void InicializarInimigo(int x, int y)
{
    g_inimigo.posicao = (Vector2){ 
        (float)x, (float)y 
    };
    g_inimigo.velocidade = (Vector2){ 0, 0 };
    g_inimigo.velocidadeLinear = 0.0f; // <--- Isso permite usar aceleração depois

    g_inimigo.angulo = 0.0f;
    g_inimigo.targetRotation = 0.0f;

    g_inimigo.vida = 100;
    g_inimigo.voltas = 0;

    // Velocidade de giro (o quanto a IA consegue virar o volante por frame)
    g_inimigo.velocidadeGiro = 5.0f;
    g_inimigo.origem = (Vector2){ 0.0f, 0.0f };
}
void DesenharJogador(void)
{
    //Recorte da imagem: Define que vamos usar a imagem inteira do sprite
    Rectangle sourceRec = { 0.0f, 0.0f, (float)g_jogador.sprite.width, (float)g_jogador.sprite.height };

    //Tamanho na tela: Vamos fazer o carro um pouquinho maior que o bloco (1.5x) para ficar bonito
    float destSize = BLOCK_SIZE * 1.3f;

    //Destino: Onde desenhar na tela
    Rectangle destRec = {
        g_jogador.posicao.x,
        g_jogador.posicao.y,
        destSize,
        destSize
    };

    //Pivô: Define o centro de rotação (metade do tamanho do destino)
    Vector2 origem = { destSize / 2.0f, destSize / 2.0f };

    //Desenha a textura
    DrawTexturePro(g_jogador.sprite, sourceRec, destRec, origem, g_jogador.angulo+180, WHITE);
}
void DesenharInimigo(void)
{
    //Recorte da imagem: Define que vamos usar a imagem inteira do sprite do inimigo
    Rectangle sourceRec = { 0.0f, 0.0f, (float)g_inimigo.sprite.width, (float)g_inimigo.sprite.height };

    //Tamanho na tela: Igual ao do jogador (1.5x o bloco)
    float destSize = BLOCK_SIZE * 1.3f;

    //Destino: Usa a posição do INIMIGO (g_inimigo)
    Rectangle destRec = {
        g_inimigo.posicao.x,
        g_inimigo.posicao.y,
        destSize,
        destSize
    };

    //Pivô: Centro da imagem
    Vector2 origem = { destSize / 2.0f, destSize / 2.0f };

    // Desenha a textura
    // Usa g_inimigo.sprite, g_inimigo.angulo (+90 para ajustar rotação) e cor WHITE para não tingir
    DrawTexturePro(g_inimigo.sprite, sourceRec, destRec, origem, g_inimigo.angulo, WHITE);
}
int main(void)
{

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Rock N Roll Racing"); //funcao que inicializa a janela, dentro tem (largura , altura, titulo)
    SetTargetFPS(60); // diz pro pc rodar em 60 fps, ou pelo menos tentar, caso ele n tenha essa capacidade

    // Carrega as imagens do disco
    g_jogador.sprite = LoadTexture("car_blue.png");
    g_inimigo.sprite = LoadTexture("car_red.png");

    // Configura a velocidade de giro da IA (importante inicializar aqui também)
    g_inimigo.velocidadeGiro = 5.0f;

    // 2. Carregar dados (fora do loop)
    CarregarMapa("pista1.txt");

    // 3. Game Loop
    while (!WindowShouldClose()) // Rodar o jogo enquanto a janela NAO fechar
    {
        AtualizarPosicao(); //chama a funcao de movimentacao do carro

        AtualizarInimigo();// Faz o inimigo se mover

        BeginDrawing(); // inicializa o desenho
        ClearBackground(BLACK); // "limpa" o rastro que seria deixado, entao exclui uma imagem anterior para fazer a proxima

        DesenhaMapa(); // Chama a função de desenho do mapa
        DesenharJogador(); //Chama a função que desenha o carro

        DesenharInimigo(); // Faz o inimigo aparecer na tela

        EndDrawing(); // finaliza o desenho
    }

    // 6. Finalização
    CloseWindow();
    return 0;
}