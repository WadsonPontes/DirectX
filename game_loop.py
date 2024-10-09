import pygame
import sys

# Inicializa o pygame
pygame.init()

# Definindo as dimensões da janela
largura, altura = 800, 600
count = 0
fps = "FPS"
tela = pygame.display.set_mode((largura, altura), pygame.HWSURFACE | pygame.DOUBLEBUF)

# Definindo o nome da janela
pygame.display.set_caption("Teste de FPS")

# Criando um relógio para controlar o FPS
relogio = pygame.time.Clock()

# Definindo uma fonte para exibir o FPS
fonte = pygame.font.SysFont("Arial", 30)

# Cor de fundo
cor_fundo = (0, 0, 0)  # Preto

# Loop principal do jogo
rodando = True
while rodando:
    # Verificando eventos
    for evento in pygame.event.get():
        if evento.type == pygame.QUIT:
            rodando = False

    # Preenchendo a tela com a cor de fundo
    tela.fill(cor_fundo)

    # Obtendo o FPS atual
    if count > 1000:
        fps = int(relogio.get_fps())
        count = 0
    else:
        count += 1
    
    # Renderizando o FPS na tela
    texto_fps = fonte.render(f"FPS: {fps}", True, (255, 255, 255))
    tela.blit(texto_fps, (10, 10))

    # Atualizando a tela
    pygame.display.flip()

    # Limitando o FPS para o máximo possível
    relogio.tick()

# Encerrando o pygame
pygame.quit()
sys.exit()
