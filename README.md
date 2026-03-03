# Gerenciador de Tarefas com Servidor HTTP em C

![Demonstração](link-para-gif.gif)

Um sistema completo de lista de tarefas com backend em C e frontend em HTML/JS.

## Funcionalidades

- Servidor HTTP escrito do zero em C (sem bibliotecas externas além das padrão)
- Serve arquivos estáticos (HTML, CSS, JS)
- API REST para gerenciar tarefas (GET, POST)
- Persistência em arquivo JSON
- Interface web responsiva

## Como executar

1. Clone o repositório
2. Compile: `make`
3. Execute: `./server`
4. Acesse `http://localhost:8080`

## Tecnologias

- C (sockets, manipulação de arquivos)
- HTML, CSS, JavaScript (frontend)
- JSON para dados

## Aprendizados

- Criação de um servidor web básico e entendimento do protocolo HTTP
- Manipulação de requisições e respostas em C
- Integração entre backend e frontend com JavaScript
- Persistência de dados em arquivo
