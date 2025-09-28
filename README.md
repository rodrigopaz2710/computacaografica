# 1) Abra o terminal "MSYS2 UCRT64" (ou MINGW64)
# 2) Instale (uma vez) os pacotes necessários:
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut

# 3) Vá até a pasta do arquivo algoritmopintor.c:
cd /c/Users/SEU_USUARIO/Desktop/redbook   # (ajuste o caminho)

# 4) Compile:
gcc algoritmopintor.c -o algoritmopintor -lfreeglut -lopengl32 -lglu32

# 5) Execute:
./algoritmopintor


3) O que o programa faz 

Renderiza uma cena 3D simples (piso texturizado, paredes, janela semitransparente e um cubo) e permite comparar Algoritmo Pintor (ordenação por profundidade e desenho back-to-front) com o z-buffer.

A câmera se move por uma curva de Bézier cúbica (controlada por t, teclas +/-).

Inclui iluminação (GL_LIGHT0), textura procedural (tabuleiro checker), back-face culling, alpha blending e sombreamento SMOOTH.

Você pode ativar/desativar cada técnica para observar efeitos de oclusão, ordem de desenho e artefatos.


4) Principais problemas (e como resolvemos)

Transparência x z-buffer: faces semitransparentes “quebram” com z-buffer puro. Solução: desenhá-las ordenadas de trás para frente (pintor) ou mover os objetos para evitar interpenetração mal definida.

Ordenação correta: o pintor funciona melhor com profundidade em coordenadas de olho. Pegamos a modelview e transformamos cada vértice para o espaço da câmera, usando a média de z do polígono como chave de ordenação.

Interpenetração: se dois polígonos se atravessam, o pintor simples falha (ordem parcial). O exemplo evita esse caso para fins didáticos.

Toolchain no Windows: certifique-se de usar o terminal MSYS2 UCRT64/MINGW64 e instalar gcc e freeglut corretos.


5) O que poderia melhorar

Separar polígonos opacos e semitransparentes: desenhar opacos com z-buffer primeiro e, depois, translúcidos back-to-front (mantendo z-buffer em read-only).

Implementar um particionamento espacial (BSP/Octree) para ordenação mais robusta em cenas grandes.

Carregar texturas de arquivo (ex.: stb_image.h) e modelos (OBJ) para deixar a cena mais rica.

Migrar para VAOs/VBOs e pipeline moderno (GL 3+) quando sair do escopo da disciplina.
