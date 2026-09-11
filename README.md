# Guia para compilação do código
Para compilar o código, é necessário que você tenha um compilador de C/C++ instalado no seu computador, eu particularmente instalei pelo pacote msys2.

Com o compilador em mãos, faça o download do arquivo edb22.cpp e abra o terminal na pasta onde se localiza o arquivo.

Caso queira fazer mudanças na entrada do código, vá ao main e altere os valores em std::vector<int> tamanhos_n = {...}

Estando na pasta correta dentro do terminal, cole esse código lá: g++ -O2 -std=c++17 edb22.cpp -o experimento2

Ele criará um executável Experimento2, em seguida digite ./Experimento2 e o código deverá começar a rodar.
