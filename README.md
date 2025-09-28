Descrição

Este projeto implementa o Algoritmo Pintor em C/OpenGL. O objetivo é desenhar polígonos na ordem correta de profundidade (do mais distante para o mais próximo), simulando a técnica clássica de resolução de visibilidade em computação gráfica.
Além da ordenação manual, também foram feitos testes com Z-buffer para comparação. O projeto utiliza elementos de todas as aulas práticas da disciplina.

Como compilar e executar

No terminal MSYS2 (mingw64) dentro da pasta do projeto, use os comandos:

gcc algoritmopintor.c -o algoritmopintor -lfreeglut -lopengl32 -lglu32
./algoritmopintor

Conteúdos utilizados das Aulas Práticas

Aula Prática 01 (hello.c): Estrutura básica de janela, display, reshape e teclado.

Aula Prática 02 (cube.c / robot.c): Transformações (glTranslatef, glRotatef, gluLookAt) e noções de oclusão/visibilidade.

Aula Prática 03 (light.c / movelight.c): Iluminação (luz posicional e spotlight) aplicada aos objetos para realçar profundidade.

Aula Prática 04 (checker.c / stb_image): Texturas aplicadas a objetos para enriquecer a cena e demonstrar repetição e filtragem.

Aula Prática 05 (bezcurve.c / bezmsh.c): Uso de curvas e superfícies como elemento extra para compor o cenário.

Contribuição dos Integrantes

Rodrigo:
Responsável pela implementação do código principal em algoritmopintor.c. Desenvolveu a lógica do Algoritmo Pintor (ordenação por profundidade), programou as transformações e coordenadas da câmera, configurou iluminação e texturas, além de realizar os testes de compilação e execução no Windows/MSYS2.

Matheus:
Responsável por documentação e refinamento. Adicionou comentários explicativos no código, organizou este README, relacionou os conteúdos das aulas práticas com o projeto, e realizou testes de cena alterando posições e ângulos para validar a lógica do Algoritmo Pintor em comparação ao Z-buffer.

Como interagir

Teclas específicas permitem movimentar objetos, alterar ordem de desenho e ativar/desativar recursos de profundidade.

Isso possibilita observar claramente os efeitos de oclusão e a diferença entre Algoritmo Pintor e Z-buffer.
