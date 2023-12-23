Este projeto consiste na implementação em c++ de DCEL (Doubly Connected Edge List) e da demonstração da manutenção de suas propriedades ao inserir novas arestas com o uso do OpenGL.

Existem quatro estados de processamento do programa: Edge Inclusion, Vertex Inclusion, Vertex Orbit e Show Face Edges. 

Edge Inclusion é o estado inicial do programa. Porém, é possível mudar para ele por meio da tecla "e". Esse estado permite a inclusão de vértices nas regiões clicadas com o uso do mouse e a criação de aresta por meio do clique em sequência de dois vértices já existentes.

Vertex Inclusion é o estado no qual o programa fica no momento em que se clica na tecla "v". Ele permite a inclusão de vértices no meio de arestas que já existem sem que, para isso, as propriedades da DCEL sejam alteradas. Para realizar essa operação, basta clicar no ponto em determinada aresta em que se deseja colocar o vértice.

Vertex Orbit é o estado que permite a execução da animação que mostra as arestas que compõem a orbita de um vértice clicado com o mouse. Tais arestas são mostradas na cor vermelha. Para mudar para esse estado é preciso clicar na tecla "o".

Show Face Edges é o estado que permite a execução da animação que mostra as semi-arestas que compõem uma determinada face. É preciso clicar em sequência no vértice de origem e no de destino de uma semi-aresta que compõe a face (a face de uma determinada semi-aresta é aquela que está à sua esquerda). Para acessar esse estado, é necessário clicar na tecla "f".

É possível reiniciar tudo, o que limpa a DCEL, ao clicar na tecla "c".

Para executar o programa, é necessário executar o comando "make" e após "./dcel"
